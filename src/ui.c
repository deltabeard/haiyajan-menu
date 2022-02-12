/**
 * UI toolkit for SDL2.
 * Copyright (c) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#include <cache.h>
#include <font.h>
#include <hedley.h>
#include <SDL.h>
#include <stretchy_buffer.h>
#include <ui.h>

static const float dpi_reference = 96.0f;

static const SDL_Colour text_colour_light = {
	0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE
};

struct ui_ctx {
	/* Required to recreate texture on resizing. */
	SDL_Renderer *ren;

	/* Texture to render user interface on. */
	SDL_Texture *tex;
	/* Texture for static elements that do not change on each frame. */
	SDL_Texture *static_tex;

	/* Root Menu. */
	const struct ui_element *root;

	/* Currently rendered menu. */
	const struct ui_element *current;

	/* Currently selected menu item. */
	const struct ui_element *selected;

	/* Cache of elements. */
	struct cache_ctx *cache;

	/* Font context used to draw text on UI elements. */
	font_ctx_s *font;

	/* Rendered input boxes for touch and mouse input. */
	struct hit_box {
		/* Dimensions of hit-box on screen. */
		SDL_Rect hit_box;

		/* UI element associated with hit-box. */
		const struct ui_element *ui_element;
	} *hit_boxes;

	/* DPI that tex texture is rendered for. */
	float dpi;
	unsigned hdpi, vdpi;
	float dpi_multiply;

	/* Vertical screen offset in pixels, used for scrolling elements. */
	struct {
		Sint32 px_y;
		Sint32 px_requested_y;
		Uint32 last_update_ms;
	} offset;

	/* Whether the front-end must call ui_render_frame(). */
	SDL_bool redraw;

	SDL_Rect selection_square;

	struct
	{
		Uint8 label;
		Uint8 tile;
	} padding;

	Uint16 ref_tile_size;
};

typedef enum {
	/* Go back to the previous item.
	 * Could be used when user presses UP. */
	MENU_INSTR_PREV_ITEM,

	/* Go to next item in menu.
	 * Could be used when user presses DOWN. */
	MENU_INSTR_NEXT_ITEM,

	/* Go to parent menu if one exists.
	 * Could be used when user presses BACKSPACE. */
	MENU_INSTR_PARENT_MENU,

	/* Execute item operation.
	 * Could be used when user presses ENTER. */
	MENU_INSTR_EXEC_ITEM
} menu_instruction_e;

static const char *elem_type_str[] = {
	"End", "Label", "Tile", "Bar"
};

/**
 * Find the next selectable UI element. Currently, this finds the next tile
 * element, as that is the only interactable element currently implemented. If
 * there are no selectable elements after and including the reference element,
 * then a previous element will be returned. Otherwise, ui_reference is
 * returned.
 * \param ui_start First element in menu.
 * \param ui_reference Element to start looking from.
 * \return Returns the next UI element of type tile. If there is no next tile:
 *	then ui_reference is returned if it is a tile,
 *	else a previous UI element of type tile is returned,
 *	else ui_reference is returned regardless of its type.
 */
HEDLEY_NON_NULL(1)
static const struct ui_element *get_first_selectable_ui_element(
	const struct ui_element	*ui_start,
	const struct ui_element	*ui_reference);

/**
 * Find the previous selectable UI element.
 * \param ui_start The first element in the menu.
 * \param ui_reference The element to start looking back from.
 * \return Returns the previous UI element of type tile. If there is no
 *	previous tile, then ui_reference is returned.
 */
HEDLEY_NON_NULL(1,2)
static const struct ui_element *get_prev_selectable_ui_element(
	const struct ui_element	*ui_start,
	const struct ui_element *ui_reference);

/**
 * Scrolls the user interface. This function is executed on each from and
 * modifies the vertical offset when the value of ctx->offset.px_requested_y is
 * non-zero.
 */
static void ui_handle_offset(ui_ctx_s *ctx)
{
	const unsigned drag_y_per_ms = 2;
	Uint32 cur_ms, diff_ms;
	Sint32 diff_offset;
	Sint32 old_px_y;

	cur_ms = SDL_GetTicks();
	old_px_y = ctx->offset.px_y;

	/* Exit if no scrolling is requested. */
	if(ctx->offset.px_requested_y == 0)
	{
		ctx->offset.last_update_ms = cur_ms;
		return;
	}

	/* Calculate time since last update. */
	if(cur_ms > ctx->offset.last_update_ms)
	{
		diff_ms = cur_ms - ctx->offset.last_update_ms;
	}
	else
	{
		/* If GetTicks has overflowed, do not take last value into
		 * account. */
		diff_ms = cur_ms;
	}

	/* In the event that the time elapsed since last redraw is so
	 * significant that a signed overflow could happen later, then
	 * immediately apply the full offset. */
	if(HEDLEY_UNLIKELY(diff_ms > SDL_MAX_SINT32 / drag_y_per_ms))
	{
		ctx->offset.px_requested_y = 0;
		ctx->offset.px_y = 0;
		goto out;
	}

	diff_offset = diff_ms * drag_y_per_ms;

	if(diff_offset > SDL_abs(ctx->offset.px_requested_y))
		diff_offset = SDL_abs(ctx->offset.px_requested_y);

	/* Apply scrolling offset for next draw. */
	if(ctx->offset.px_requested_y < 0)
	{
		ctx->offset.px_requested_y += diff_offset;
		ctx->offset.px_y += diff_offset;
		if(ctx->offset.px_requested_y > 0)
			ctx->offset.px_requested_y = 0;
	}
	else if(ctx->offset.px_requested_y > 0)
	{
		ctx->offset.px_requested_y -= diff_offset;
		ctx->offset.px_y -= diff_offset;
		if(ctx->offset.px_requested_y < 0)
			ctx->offset.px_requested_y = 0;
	}

	/* Do not allow scrolling further up from the first element. */
	if(ctx->offset.px_y < 0)
	{
		ctx->offset.px_y = 0;
	}
	else
	{
		/* Make sure that user doesn't scroll past last element. */
		const struct hit_box h = sb_last(ctx->hit_boxes);
		int y_thresh;
		int disp_height;

		SDL_GetRendererOutputSize(ctx->ren, NULL, &disp_height);
		y_thresh = disp_height - (ctx->ref_tile_size * 2);

		/* If last element is above the vertical threshold, pull it
		 * back. */
		if(h.hit_box.y < y_thresh)
		{
			ctx->offset.px_requested_y = (y_thresh - h.hit_box.y);
		}
	}

out:
	ctx->offset.last_update_ms = cur_ms;

	/* Only redraw if offset has changed. */
	if(ctx->offset.px_y != old_px_y)
		ctx->redraw = SDL_TRUE;

	return;
}

HEDLEY_NON_NULL(1)
static void ui_input(ui_ctx_s *ctx, menu_instruction_e instr)
{
	switch(instr)
	{
	case MENU_INSTR_PREV_ITEM:
		/* Only select previous item if it isn't the first. */
		ctx->selected = get_prev_selectable_ui_element(ctx->current,
			ctx->selected);
		break;

	case MENU_INSTR_NEXT_ITEM:
		ctx->selected++;
		ctx->selected = get_first_selectable_ui_element(ctx->current,
				ctx->selected);
		break;

#if 0
		case MENU_INSTR_PARENT_MENU:
			if(ctx->selected->parent != NULL)
				ctx->selected = ctx->selected->parent;

			break;
#endif

	case MENU_INSTR_EXEC_ITEM:
	{
		switch(ctx->selected->elem.tile.onclick.action)
		{
		case UI_EVENT_GOTO_ELEMENT:
			ctx->current = ctx->selected->elem.tile.onclick.action_data.goto_element.element;

			/* Set the selected item as the first selectable item
			 * in the new menu. */
			ctx->selected = get_first_selectable_ui_element(ctx->current,
					ctx->current);
			break;

		case UI_EVENT_EXECUTE_FUNCTION:
			ctx->selected->elem.tile.onclick.action_data.execute_function.function(ctx->selected);
			break;

		case UI_EVENT_SET_SIGNED_VARIABLE:
			*ctx->selected->elem.tile.onclick.action_data.signed_variable.variable =
				ctx->selected->elem.tile.onclick.action_data.signed_variable.val;
			break;

		case UI_EVENT_SET_UNSIGNED_VARIABLE:
			*ctx->selected->elem.tile.onclick.action_data.unsigned_variable.variable =
				ctx->selected->elem.tile.onclick.action_data.unsigned_variable.val;
			break;

		case UI_EVENT_NOP:
		default:
			/* Do not redraw on no operation. */
			return;
		}

		break;
	}
	}

	SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "Selected item %s",
			elem_type_str[ctx->selected->type]);
	ctx->redraw = SDL_TRUE;
	return;
}

static void ui_resize_all(ui_ctx_s *ui, int win_w, int win_h)
{
	const float icon_size_reference = 40.0f;
	const float header_size_ref = 28.0f;
	const float regular_size_ref = 16.0f;
	int icon_pt, header_pt, regular_pt;
	float dpi_multiply_h = ui->dpi_multiply,
		dpi_multiply_w = ui->dpi_multiply;

	const Sint32 dpi_scale_thresh_h = UI_MIN_WINDOW_HEIGHT * 2;
	const Sint32 dpi_scale_thresh_w = UI_MIN_WINDOW_WIDTH * 2;

	SDL_assert(ui->dpi > 0.0f);
	SDL_assert(ui->dpi_multiply > 0.0f);
	SDL_assert(win_h >= UI_MIN_WINDOW_HEIGHT);
	SDL_assert(win_w >= UI_MIN_WINDOW_WIDTH);

	/* Limit minimum tile size. */
	if(win_h <= UI_MIN_WINDOW_HEIGHT)
		win_h = UI_MIN_WINDOW_HEIGHT;

	if(win_w <= UI_MIN_WINDOW_WIDTH)
		win_w = UI_MIN_WINDOW_WIDTH;

	/* Reduce effect of DPI scaling if window is very small. */
	if(win_h <= dpi_scale_thresh_h)
	{
		const Sint32 min_height_scale = UI_MIN_WINDOW_HEIGHT / 4;
		float dpi_scale = (float)(win_h - min_height_scale) /
			(float)(dpi_scale_thresh_h - min_height_scale);
		dpi_multiply_h = ui->dpi_multiply * dpi_scale;
	}

	if(win_w <= dpi_scale_thresh_w)
	{
		const Sint32 min_width_scale = UI_MIN_WINDOW_WIDTH / 4;
		float dpi_scale = (float)(win_w - min_width_scale) /
			(float)(dpi_scale_thresh_w - min_width_scale);
		dpi_multiply_w = ui->dpi_multiply * dpi_scale;
	}

	if(dpi_multiply_h != ui->dpi_multiply ||
			dpi_multiply_w != ui->dpi_multiply)
	{
		if(dpi_multiply_h < dpi_multiply_w)
		{
			ui->dpi_multiply = dpi_multiply_h;
		}
		else
		{
			ui->dpi_multiply = dpi_multiply_w;
		}
		SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
			"DPI scaling changed to %f", ui->dpi_multiply);
	}

	/* Set reference padding between elements. */
	ui->padding.label = (Uint8)(8.0f * ui->dpi_multiply);
	ui->padding.tile = (Uint8)(16.0f * ui->dpi_multiply);
	SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
		"Tile padding changed to %d", ui->padding.tile);

	/* Set reference tile size. */
	ui->ref_tile_size = (Uint16)(100.0f * ui->dpi_multiply);
	SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
		"Reference tile size changed to %d", ui->ref_tile_size);

	icon_pt = (int)(icon_size_reference);
	header_pt = (int)(header_size_ref);
	regular_pt = (int)(regular_size_ref);

	SDL_assert(ui->font != NULL);
	font_change_pt(ui->font,
		(float)ui->hdpi * ui->dpi_multiply,
		(float)ui->vdpi * ui->dpi_multiply,
		icon_pt, header_pt, regular_pt);
	clear_cached_textures(ui->cache);
}

HEDLEY_NON_NULL(1,2)
void ui_process_event(ui_ctx_s *HEDLEY_RESTRICT ctx, SDL_Event *HEDLEY_RESTRICT e)
{
	/* Recalculate begin_actual coordinates on resolution and DPI change. */
	if(e->type == SDL_KEYDOWN)
	{
		switch(e->key.keysym.sym)
		{
		case SDLK_w:
		case SDLK_UP:
			ui_input(ctx, MENU_INSTR_PREV_ITEM);
			break;

		case SDLK_s:
		case SDLK_DOWN:
			ui_input(ctx, MENU_INSTR_NEXT_ITEM);
			break;

		case SDLK_a:
		case SDLK_LEFT:
			/* TODO: Add skip items forward and back. */
			//ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
			break;

		case SDLK_d:
		case SDLK_RIGHT:
			//ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
			break;

		case SDLK_SPACE:
		case SDLK_RETURN:
		case SDLK_z:
			ui_input(ctx, MENU_INSTR_EXEC_ITEM);
			break;

		case SDLK_x:
		case SDLK_BACKSPACE:
			ui_input(ctx, MENU_INSTR_PARENT_MENU);
			break;
		}
	}
	else if(e->type == SDL_WINDOWEVENT)
	{
		SDL_Window *win;

		win = SDL_GetWindowFromID(e->window.windowID);
		if(win == NULL)
		{
			SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
				"Unable to obtain window from ID %d: %s",
				e->window.windowID, SDL_GetError());
			return;
		}

		switch(e->window.event)
		{
		case SDL_WINDOWEVENT_MOVED:
		{
			int display_id = SDL_GetWindowDisplayIndex(win);
			float new_dpi, new_hdpi, new_vdpi;
			int h, w;

			if(SDL_GetDisplayDPI(display_id,
					&new_dpi, &new_hdpi, &new_vdpi) < 0)
			{
				new_dpi = dpi_reference;
				new_hdpi = dpi_reference;
				new_vdpi = dpi_reference;
			}

			if(new_dpi == ctx->dpi)
				break;

			ctx->dpi = new_dpi;
			ctx->hdpi = (unsigned)SDL_ceilf(new_hdpi);
			ctx->vdpi = (unsigned)SDL_ceilf(new_vdpi);
			ctx->dpi_multiply = ctx->dpi / dpi_reference;

			SDL_GetWindowSize(win, &w, &h);
			ui_resize_all(ctx, w, h);
		}
			break;

		case SDL_WINDOWEVENT_RESIZED:
		{
			SDL_Renderer *ren;
			SDL_Texture *new_tex, *new_static_tex;
			Uint32 texture_format;
			Sint32 new_w, new_h;

			ren = SDL_GetRenderer(win);
			if(ren == NULL)
			{
				SDL_LogDebug(SDL_LOG_CATEGORY_RENDER,
					"Unable to obtain renderer from window: %s",
					SDL_GetError());
				return;
			}

			texture_format = SDL_GetWindowPixelFormat(win);
			new_w = e->window.data1;
			new_h = e->window.data2;

			new_tex = SDL_CreateTexture(ren, texture_format,
				SDL_TEXTUREACCESS_TARGET,
				new_w, new_h);
			if(new_tex == NULL)
			{
				SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
					"Unable to create new texture: %s",
					SDL_GetError());
				return;
			}

			new_static_tex = SDL_CreateTexture(ren, texture_format,
				SDL_TEXTUREACCESS_TARGET,
				new_w, new_h);
			if(new_static_tex == NULL)
			{
				SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
					"Unable to create new texture for "
					"static elements: %s",
					SDL_GetError());
				return;
			}

			SDL_DestroyTexture(ctx->tex);
			SDL_DestroyTexture(ctx->static_tex);
			ctx->tex = new_tex;
			ctx->static_tex = new_static_tex;

			ctx->dpi_multiply = ctx->dpi / dpi_reference;

			ui_resize_all(ctx, new_w, new_h);

			SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
				"Successfully resized texture size to %dW %dH",
				new_w, new_h);
		}

			break;

		default:
			return;
		}

		ctx->redraw = SDL_TRUE;
	}
	else if(e->type == SDL_MOUSEMOTION && ctx->hit_boxes != NULL)
	{
		SDL_Point p = {
			.x = e->motion.x,
			.y = e->motion.y
		};
		int boxes_n = sb_count(ctx->hit_boxes);

		for(int box = 0; box < boxes_n; box++)
		{
			struct hit_box *this_box = &ctx->hit_boxes[box];
			const SDL_Rect *r = &this_box->hit_box;
			SDL_bool intersects = SDL_PointInRect(&p, r);

			if(intersects == SDL_FALSE)
				continue;

			if(ctx->selected != this_box->ui_element)
			{
				ctx->redraw = SDL_TRUE;
				ctx->selected = this_box->ui_element;
				SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
					"Selected item %p using motion",
					(void *)ctx->selected);
			}

			break;
		}
	}
	else if(e->type == SDL_MOUSEBUTTONUP && ctx->hit_boxes != NULL)
	{
		SDL_Point p = {
			.x = e->button.x,
			.y = e->button.y
		};

		Uint32 boxes_n = sb_count(ctx->hit_boxes);

		if(e->button.button != SDL_BUTTON_LEFT)
			return;

		if(e->button.clicks == 0)
			return;

		for(Uint32 box = 0; box < boxes_n; box++)
		{
			struct hit_box *this_box = &ctx->hit_boxes[box];
			const SDL_Rect *r = &this_box->hit_box;
			SDL_bool intersects = SDL_PointInRect(&p, r);

			if(intersects == SDL_FALSE)
				continue;

			if(ctx->selected != this_box->ui_element)
			{
				ctx->redraw = SDL_TRUE;
				ctx->selected = this_box->ui_element;
				SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
					"Selected item %p using button",
					(void *)ctx->selected);
			}

			ui_input(ctx, MENU_INSTR_EXEC_ITEM);
			SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
				"Executed item %p using button",
				(void *)ctx->selected);

			break;
		}
	}
	else if(e->type == SDL_MOUSEWHEEL)
	{
		if(e->wheel.y > 0)
			ui_input(ctx, MENU_INSTR_PREV_ITEM);
		else if(e->wheel.y < 0)
			ui_input(ctx, MENU_INSTR_NEXT_ITEM);
		return;
	}

	return;
}

HEDLEY_NON_NULL(1,2)
static void ui_draw_selection(ui_ctx_s *HEDLEY_RESTRICT ctx,
		const SDL_Rect *HEDLEY_RESTRICT r)
{
	/* Offset the selection square to surround the selection from the
	 * outside. */
	Uint32 offset = (unsigned)(2.0f * ctx->dpi_multiply) + 1;
	/* Set the dimensions of the selection square. */
	SDL_Rect outline = {
		.x = r->x - offset,
		.y = r->y - offset,
		.h = r->h + (offset * 2),
		.w = r->w + (offset * 2)
	};
	/* Set the thickness of the selection square. */
	const unsigned thickness = 1 + (unsigned)SDL_ceilf(5.0f * ctx->dpi_multiply);
	//const SDL_Colour bright = { 0x19, 0x82, 0xC4, 0xFF };
	//const SDL_Colour dark = { 0x10, 0x54, 0x7E, 0xFF };
	static const Uint8 red[256] = {
		15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17,
		17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18,
		18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		20, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18,
		18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 17,
		17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13,
		13, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12,
		12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12,
		12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13,
		13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15
	};
	static const Uint8 green[256] = {
		126, 127, 128, 129, 131, 132, 133, 134, 135, 136, 137, 138, 139,
		140, 141, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 152,
		153, 154, 155, 156, 157, 158, 159, 159, 160, 161, 162, 162, 163,
		164, 164, 165, 165, 166, 167, 167, 168, 168, 168, 169, 169, 170,
		170, 170, 171, 171, 171, 171, 172, 172, 172, 172, 172, 172, 172,
		172, 172, 172, 172, 172, 172, 171, 171, 171, 171, 170, 170, 170,
		169, 169, 168, 168, 168, 167, 167, 166, 165, 165, 164, 164, 163,
		162, 162, 161, 160, 159, 159, 158, 157, 156, 155, 154, 153, 152,
		152, 151, 150, 149, 148, 147, 146, 145, 144, 143, 141, 140, 139,
		138, 137, 136, 135, 134, 133, 132, 131, 129, 128, 127, 126, 125,
		124, 123, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111,
		109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 100, 99, 98,
		97, 96, 95, 94, 93, 93, 92, 91, 90, 90, 89, 88, 88, 87, 87, 86,
		85, 85, 84, 84, 84, 83, 83, 82, 82, 82, 81, 81, 81, 81, 80, 80,
		80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 81, 81, 81, 81, 82,
		82, 82, 83, 83, 84, 84, 84, 85, 85, 86, 87, 87, 88, 88, 89, 90,
		90, 91, 92, 93, 93, 94, 95, 96, 97, 98, 99, 100, 100, 101, 102,
		103, 104, 105, 106, 107, 108, 109, 111, 112, 113, 114, 115, 116,
		117, 118, 119, 120, 121, 123, 124, 125
	};
	static const Uint8 blue[256] = {
		189, 190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199,
		200, 201, 202, 202, 203, 204, 205, 205, 206, 207, 208, 208, 209,
		210, 211, 211, 212, 213, 213, 214, 214, 215, 216, 216, 217, 217,
		218, 218, 219, 219, 219, 220, 220, 221, 221, 221, 222, 222, 222,
		222, 223, 223, 223, 223, 223, 224, 224, 224, 224, 224, 224, 224,
		224, 224, 224, 224, 224, 224, 223, 223, 223, 223, 223, 222, 222,
		222, 222, 221, 221, 221, 220, 220, 219, 219, 219, 218, 218, 217,
		217, 216, 216, 215, 214, 214, 213, 213, 212, 211, 211, 210, 209,
		208, 208, 207, 206, 205, 205, 204, 203, 202, 202, 201, 200, 199,
		198, 198, 197, 196, 195, 194, 193, 192, 192, 191, 190, 189, 188,
		187, 186, 186, 185, 184, 183, 182, 181, 180, 180, 179, 178, 177,
		176, 176, 175, 174, 173, 173, 172, 171, 170, 170, 169, 168, 167,
		167, 166, 165, 165, 164, 164, 163, 162, 162, 161, 161, 160, 160,
		159, 159, 159, 158, 158, 157, 157, 157, 156, 156, 156, 156, 155,
		155, 155, 155, 155, 154, 154, 154, 154, 154, 154, 154, 154, 154,
		154, 154, 154, 154, 155, 155, 155, 155, 155, 156, 156, 156, 156,
		157, 157, 157, 158, 158, 159, 159, 159, 160, 160, 161, 161, 162,
		162, 163, 164, 164, 165, 165, 166, 167, 167, 168, 169, 170, 170,
		171, 172, 173, 173, 174, 175, 176, 176, 177, 178, 179, 180, 180,
		181, 182, 183, 184, 185, 186, 186, 187, 188};
	unsigned col_factor = (SDL_GetTicks() % 1024) / 4;
	SDL_Colour sel_col;

	sel_col.r = red[col_factor];
	sel_col.g = green[col_factor];
	sel_col.b = blue[col_factor];
	sel_col.a = 0xFF;
	SDL_SetRenderDrawColor(ctx->ren, sel_col.r, sel_col.g, sel_col.b,
		sel_col.a);

	for(unsigned i = 0; i < thickness; i++)
	{
		SDL_RenderDrawRect(ctx->ren, &outline);
		outline.x++;
		outline.y++;
		outline.h -= 2;
		outline.w -= 2;
	}

	/* Check if hit box is offscreen. */
	{
		SDL_Rect screen;
		screen.x = 0;
		screen.y = 0;
		SDL_GetRendererOutputSize(ctx->ren, &screen.w, &screen.h);
		if(outline.y + outline.h > screen.h)
			ctx->offset.px_requested_y = -(ctx->ref_tile_size);
		else if(outline.y < 0)
			ctx->offset.px_requested_y = ctx->ref_tile_size;
	}

	return;
}

/**
 * Draw label element 'el' at point 'p'.
 *
 * \param ctx	UI context.
 * \param el	UI element parameters.
 * \param p	The top left point of the UI element to draw.
*/
HEDLEY_NON_NULL(1,2,3)
static void ui_draw_label(ui_ctx_s *HEDLEY_RESTRICT ctx,
	const struct ui_element *HEDLEY_RESTRICT el,
	SDL_Point *HEDLEY_RESTRICT p)
{
	SDL_Texture *label_tex;
	SDL_Rect dim = {
		.x = p->x, .y = p->y
	};

	/* Render text. */
	label_tex = get_cached_texture(ctx->cache,
		&el->elem.label, sizeof(el->elem.label));
	if(label_tex == NULL)
	{
		label_tex = font_render_text(ctx->font, el->elem.label.label,
			el->elem.label.style, FONT_QUALITY_HIGH,
			text_colour_light);
		/* FIXME: Missing checks. */
		store_cached_texture(ctx->cache, &el->elem.label,
			sizeof(el->elem.label), label_tex);
	}

	SDL_QueryTexture(label_tex, NULL, NULL, &dim.w, &dim.h);
	SDL_RenderCopy(ctx->ren, label_tex, NULL, &dim);

	/* Increment coordinates to next element. */
	p->y += dim.h + ctx->padding.label;

	return;
}

/**
 * Draw tile element 'el' at point 'p'.
 *
 * \param ctx	UI context.
 * \param el	UI element parameters.
 * \param p	The top left point of the UI element to draw.
*/
HEDLEY_NON_NULL(1,2,3)
static void ui_draw_tile(ui_ctx_s *HEDLEY_RESTRICT ctx,
		const struct ui_element *HEDLEY_RESTRICT el,
		SDL_Point *HEDLEY_RESTRICT p)
{
	const Uint16 len = ctx->ref_tile_size;
	const SDL_Rect dim = {
		.h = len, .w = len, .x = p->x, .y = p->y
	};
	SDL_Texture *text_tex, *icon_tex;
	SDL_Rect text_dim, icon_dim;
	const SDL_Point tile_padding = {
		.x = ctx->padding.tile,
		.y = ctx->padding.tile
	};

	/* Draw tile background. */
	SDL_SetRenderDrawColor(ctx->ren,
		el->elem.tile.bg.r, el->elem.tile.bg.g,
		el->elem.tile.bg.b, el->elem.tile.bg.a);
	SDL_RenderFillRect(ctx->ren, &dim);

	/* Render icon on tile. */
	icon_tex = get_cached_texture(ctx->cache, &el->elem.tile.icon, sizeof(el->elem.tile.icon));
	if(icon_tex == NULL)
	{
		icon_tex = font_render_icon(ctx->font, el->elem.tile.icon,
				el->elem.tile.fg);
		/* FIXME: Missing checks. */
		store_cached_texture(ctx->cache, &el->elem.tile.icon,
				sizeof(el->elem.tile.icon), icon_tex);
	}
	SDL_QueryTexture(icon_tex, NULL, NULL, &icon_dim.w, &icon_dim.h);

	/* If the icon texture is larger than the tile itself (due to the
	 * cached icon being high resolution) then resize the icon until it
	 * fits within the tile. */
	while(icon_dim.w + ctx->padding.tile >= dim.w)
	{
		icon_dim.w /= 2;
		icon_dim.h /= 2;
	}
	icon_dim.x = p->x + (len / 2) - (icon_dim.w / 2);
	icon_dim.y = p->y + (len / 2) - (icon_dim.h / 2);

	SDL_SetTextureColorMod(icon_tex,
		el->elem.tile.fg.r, el->elem.tile.fg.g,
		el->elem.tile.fg.b);
	SDL_RenderCopy(ctx->ren, icon_tex, NULL, &icon_dim);
	//SDL_DestroyTexture(icon_tex);

	/* Render tile label. */
	text_tex = get_cached_texture(ctx->cache, el->elem.tile.label,
				      SDL_strlen(el->elem.tile.label));
	if(text_tex == NULL)
	{
		text_tex = font_render_text(ctx->font, el->elem.tile.label,
					FONT_STYLE_HEADER, FONT_QUALITY_HIGH,
					text_colour_light);
		/* FIXME: Missing checks. */
		store_cached_texture(ctx->cache, el->elem.tile.label,
					SDL_strlen(el->elem.tile.label), text_tex);
	}
	SDL_QueryTexture(text_tex, NULL, NULL, &text_dim.w, &text_dim.h);

	switch(el->elem.tile.label_placement)
	{
	case LABEL_PLACEMENT_OUTSIDE_RIGHT_TOP:
		text_dim.x = p->x + len + tile_padding.x;
		text_dim.y = p->y;
		break;

	case LABEL_PLACEMENT_OUTSIDE_RIGHT_MIDDLE:
		text_dim.x = p->x + len + tile_padding.x;
		text_dim.y = p->y + (len / 2) - (text_dim.h / 2);
		break;

	case LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM:
		text_dim.x = p->x + len + tile_padding.x;
		text_dim.y = p->y + len - text_dim.h;
		break;

	default:
		HEDLEY_UNREACHABLE();
		break;
	}

	/* Colour of elements within tile. */
	SDL_SetTextureColorMod(text_tex,
		el->elem.tile.fg.r,
		el->elem.tile.fg.g,
		el->elem.tile.fg.b);

	SDL_RenderCopy(ctx->ren, text_tex, NULL, &text_dim);

	/* Add hitbox for mouse and touch input. */
	{
		struct hit_box i;
		i.hit_box = dim;
		i.ui_element = el;
		sb_push(ctx->hit_boxes, i);

		SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
			"Hit box generated for tile at (%d, %d)(%d, %d)",
			dim.x, dim.y, dim.h, dim.w);
	}

	/* Draw outline and translucent box if tile is selected. */
	if(ctx->selected == el)
	{
		ctx->selection_square.x = dim.x;
		ctx->selection_square.y = dim.y;
		ctx->selection_square.h = dim.h;
		ctx->selection_square.w = dim.w;
	}

	/* Increment coordinates to next element. */
	p->y += len + tile_padding.y;
}

HEDLEY_NON_NULL(1)
SDL_Texture *ui_render_frame(ui_ctx_s *ctx)
{
	SDL_Point vert;

	SDL_assert(ctx->tex != NULL);
	SDL_assert(ctx->static_tex != NULL);

	/* Check if any animations need to be rendered. */
	ui_handle_offset(ctx);

	if(ctx->redraw == SDL_FALSE)
		goto out;

	/* Initialise a new hitbox array. */
	if(ctx->hit_boxes != NULL)
	{
		sb_free(ctx->hit_boxes);
		ctx->hit_boxes = NULL;
	}

	if(SDL_SetRenderTarget(ctx->ren, ctx->static_tex) != 0)
		return NULL;

	/* Calculate where the first element should appear vertically. */
	SDL_GetRendererOutputSize(ctx->ren, &vert.x, &vert.y);
	vert.x /= 8;
	vert.y /= 16;
	vert.y -= ctx->offset.px_y;

	SDL_SetRenderDrawColor(ctx->ren, 20, 20, 20, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->ren);

	for(const struct ui_element *el = ctx->current;
			el->type != UI_ELEM_TYPE_END; el++)
	{
		switch(el->type)
		{
		case UI_ELEM_TYPE_LABEL:
			ui_draw_label(ctx, el, &vert);
			break;

		case UI_ELEM_TYPE_TILE:
			ui_draw_tile(ctx, el, &vert);
			break;

		default:
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"The requested UI element %d is not implemented.",
				el->type);
			break;
		}

	}

	SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "UI Rendered");
	ctx->redraw = SDL_FALSE;

out:
	/* Redraw any dynamic elements. */
	if(SDL_SetRenderTarget(ctx->ren, ctx->tex) != 0)
		return NULL;

	/* Copy static elements to output texture. */
	SDL_RenderCopy(ctx->ren, ctx->static_tex, NULL, NULL);

	ui_draw_selection(ctx, &ctx->selection_square);

	return ctx->tex;
}

HEDLEY_NON_NULL(1)
static const struct ui_element *get_first_selectable_ui_element(
	const struct ui_element	*ui_start,
	const struct ui_element	*ui_reference)
{
	const struct ui_element *e = ui_reference;

	while(e->type != UI_ELEM_TYPE_TILE && e->type != UI_ELEM_TYPE_END)
		e++;

	/* If selectable element, return. */
	if(e->type == UI_ELEM_TYPE_TILE)
		goto out;

	/* No selectable elements remain, so traverse backwards. */
	e = get_prev_selectable_ui_element(ui_start, ui_reference);

out:
	return e;
}

HEDLEY_NON_NULL(1,2)
static const struct ui_element *get_prev_selectable_ui_element(
	const struct ui_element	*ui_start, const struct ui_element
		*ui_reference)
{
	const struct ui_element *e = ui_reference;

	if(e == ui_start)
		return e;

	do {
		e--;
	} while(e->type != UI_ELEM_TYPE_TILE && e > ui_start);

	if(e->type != UI_ELEM_TYPE_TILE)
		e = ui_reference;

	return e;
}


HEDLEY_NON_NULL(1,6)
HEDLEY_MALLOC
static ui_ctx_s *ui_init_renderer(SDL_Renderer *HEDLEY_RESTRICT rend,
		float dpi, float hdpi, float vdpi, Uint32 format,
		const struct ui_element *HEDLEY_RESTRICT ui_elements)
{
	int w, h;
	ui_ctx_s *ctx;

	SDL_assert_paranoid(rend != NULL);

	ctx = SDL_calloc(1, sizeof(ui_ctx_s));
	if(ctx == NULL)
		goto err;

	ctx->ren = rend;
	if(SDL_GetRendererOutputSize(ctx->ren, &w, &h) != 0)
		goto err;

	ctx->tex = SDL_CreateTexture(ctx->ren, format,
		SDL_TEXTUREACCESS_TARGET, w, h);
	if(ctx->tex == NULL)
		goto err;

	ctx->static_tex = SDL_CreateTexture(ctx->ren, format,
		SDL_TEXTUREACCESS_TARGET, w, h);
	if(ctx->static_tex == NULL)
		goto err;

	ctx->root = ui_elements;
	ctx->current = ui_elements;
	ctx->selected = get_first_selectable_ui_element(ui_elements, ui_elements);
	ctx->redraw = SDL_TRUE;
	ctx->hit_boxes = NULL;
	ctx->dpi = dpi;
	ctx->hdpi = (unsigned)SDL_ceilf(hdpi);
	ctx->vdpi = (unsigned)SDL_ceilf(vdpi);
	ctx->dpi_multiply = dpi / dpi_reference;

	ctx->cache = init_cached_texture();

	ctx->font = font_init(rend);
	if(ctx->font == NULL)
	{
		SDL_DestroyTexture(ctx->tex);
		SDL_DestroyTexture(ctx->static_tex);
		goto err;
	}

	ui_resize_all(ctx, w, h);

	/* Draw the first frame. */
	ctx->redraw = SDL_TRUE;

out:
	return ctx;

err:
	SDL_free(ctx);
	ctx = NULL;
	goto out;
}

HEDLEY_NON_NULL(1,2)
ui_ctx_s *ui_init(SDL_Window *HEDLEY_RESTRICT win,
		const struct ui_element *HEDLEY_RESTRICT ui_elements)
{
	ui_ctx_s *ctx = NULL;
	Uint32 format;
	int display_id;
	SDL_Renderer *rend;
	float dpi, hdpi, vdpi;

	rend = SDL_GetRenderer(win);
	if(rend == NULL)
	{
		SDL_LogDebug(SDL_LOG_CATEGORY_RENDER,
			"Unable to obtain renderer from window: %s",
			SDL_GetError());
		goto err;
	}

	display_id = SDL_GetWindowDisplayIndex(win);
	if(display_id < 0)
		goto err;

	if(SDL_GetDisplayDPI(display_id, &dpi, &hdpi, &vdpi) != 0)
	{
		dpi = dpi_reference;
		hdpi = dpi_reference;
		vdpi = dpi_reference;
		SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
			"Unable to determine display DPI: %s",
			SDL_GetError());
	}

	format = SDL_GetWindowPixelFormat(win);
	ctx = ui_init_renderer(rend, dpi, hdpi, vdpi, format, ui_elements);

out:
	return ctx;

err:
	goto out;
}

HEDLEY_NON_NULL(1)
void ui_exit(ui_ctx_s *ctx)
{
	font_exit(ctx->font);

	clear_cached_textures(ctx->cache);
	deinit_cached_texture(ctx->cache);
	SDL_DestroyTexture(ctx->tex);
	SDL_DestroyTexture(ctx->static_tex);
	sb_free(ctx->hit_boxes);
	SDL_free(ctx);
}
