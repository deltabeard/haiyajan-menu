/**
 * UI toolkit for SDL2.
 * Copyright (c) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#include <font.h>
#include <SDL.h>
#include <stretchy_buffer.h>
#include <ui.h>

static const float dpi_reference = 96.0f;

static const SDL_Colour text_colour_light = {
	0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE
};

struct ui_ctx
{
	/* Required to recreate texture on resizing. */
	SDL_Renderer *ren;

	/* Texture to render on. */
	SDL_Texture *tex;

	/* Root Menu. */
	ui_el_s *root;

	/* Currently rendered menu. */
	ui_el_s *current;

	/* Font context used to draw text on UI elements. */
	font_ctx_s *font;

	/* Rendered input boxes for touch and mouse input. */
	struct hit_box
	{
		/* Dimensions of hit-box on screen. */
		SDL_Rect hit_box;

		/* UI element associated with hit-box. */
		ui_el_s *ui_element;
	} *hit_boxes;

	/* DPI that tex texture is rendered for. */
	float dpi;
	float dpi_multiply;

	/* Whether the front-end must call ui_render_frame(). */
	SDL_bool redraw;

	Uint16 ref_tile_size;
};

typedef enum
{
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

static void ui_input(ui_ctx_s *ctx, menu_instruction_e instr)
{
	switch(instr)
	{
	case MENU_INSTR_PREV_ITEM:
		if(ctx->current > ctx->root)
			ctx->current--;
		break;

	case MENU_INSTR_NEXT_ITEM:
		if((ctx->current + 1)->type != UI_ELEM_TYPE_END)
			ctx->current++;
		break;

#if 0
	case MENU_INSTR_PARENT_MENU:
		if(ctx->current->parent != NULL)
			ctx->current = ctx->current->parent;

		break;
#endif

	case MENU_INSTR_EXEC_ITEM:
	{
		switch(ctx->current->elem.tile.onclick)
		{
		case ONCLICK_GOTO_ELEMENT:
			ctx->current = ctx->current->elem.tile.onclick_event.goto_element.element;
			break;

		case ONCLICK_EXECUTE_FUNCTION:
			ctx->current->elem.tile.onclick_event.execute_function.function(ctx->current);
			break;

		case ONCLICK_SET_SIGNED_VARIABLE:
			*ctx->current->elem.tile.onclick_event.signed_variable.variable = ctx->current->elem.tile.onclick_event.signed_variable.val;
			break;

		case ONCLICK_SET_UNSIGNED_VARIABLE:
			*ctx->current->elem.tile.onclick_event.unsigned_variable.variable = ctx->current->elem.tile.onclick_event.unsigned_variable.val;
			break;
		}

		break;
	}
	}

	ctx->redraw = SDL_TRUE;
	return;
}

static void ui_set_widget_sizes(ui_ctx_s *ui, Sint32 window_height)
{
	Uint16 ref_tile_size = 100;
	float dpi_multiply = ui->dpi_multiply;
	const Sint32 dpi_scale_thresh = UI_MIN_WINDOW_HEIGHT * 4;

	SDL_assert(ui != NULL);
	SDL_assert(ui->dpi > 0.0f);
	SDL_assert(window_height >= UI_MIN_WINDOW_HEIGHT);

	/* Limit minimum tile size. */
	if(window_height <= UI_MIN_WINDOW_HEIGHT)
		window_height = UI_MIN_WINDOW_HEIGHT;

	/* Reduce effect of DPI scaling if window is very small. */
	if(window_height <= dpi_scale_thresh)
	{
		const Sint32 min_height_scale = UI_MIN_WINDOW_HEIGHT / 2;
		float dpi_scale = (window_height - min_height_scale)/
			(dpi_scale_thresh - min_height_scale);
		dpi_multiply = (dpi_multiply * dpi_scale);
	}

	ref_tile_size = (Uint32) ((float) ref_tile_size * dpi_multiply);
	ui->ref_tile_size = ref_tile_size;
}

/**
 * Calculates font sizes based upon the DPI and size of the rendering target.
 * 
 * \param dpi 
 * \param window_height 
 * \param icon_pt 
 * \param header_pt 
 * \param regular_pt 
*/
static void ui_calculate_font_sizes(ui_ctx_s *restrict ui, Sint32 window_height,
		int *restrict icon_pt, int *restrict header_pt,
		int *restrict regular_pt)
{
	static const float icon_size_reference = 46.0f;
	static const float header_size_ref = 30.0f;
	static const float regular_size_ref = 20.0f;
	float dpi_multiply = ui->dpi_multiply;
	const float dpi_scale_thresh = UI_MIN_WINDOW_HEIGHT * 4;

	/* Pedantic asserts for debug builds only. */
	SDL_assert(ui->dpi > 0.0f);
	SDL_assert(window_height >= UI_MIN_WINDOW_HEIGHT);
	SDL_assert(icon_pt != NULL);
	SDL_assert(header_pt != NULL);
	SDL_assert(regular_pt != NULL);

	/* Make sure we don't abuse "restrict" keyword. */
	SDL_assert(icon_pt != header_pt);
	SDL_assert(header_pt != regular_pt);
	SDL_assert(icon_pt != regular_pt);

	/* TODO: Do not take DPI into account for low resolution displays. */
	/* Limit minimum tile size. */
	if(window_height <= UI_MIN_WINDOW_HEIGHT)
		window_height = UI_MIN_WINDOW_HEIGHT;

	/* Reduce effect of DPI scaling if window is very small. */
	if(window_height <= dpi_scale_thresh)
	{
		const Sint32 min_height_scale = UI_MIN_WINDOW_HEIGHT / 8;
		float dpi_scale = (window_height - min_height_scale)/
				  (dpi_scale_thresh - min_height_scale);
		dpi_multiply = (dpi_multiply * dpi_scale);
	}

	*icon_pt = (int)(icon_size_reference * dpi_multiply);
	*header_pt = (int)(header_size_ref * dpi_multiply);
	*regular_pt = (int)(regular_size_ref * dpi_multiply);
}

void ui_process_event(ui_ctx_s *ctx, SDL_Event *e)
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
				float new_dpi;
				int h, w, longest;
				int icon_pt, header_pt, regular_pt;

				if(SDL_GetDisplayDPI(display_id, &new_dpi, NULL, NULL) < 0)
					new_dpi = dpi_reference;

				if(new_dpi == ctx->dpi)
					break;

				ctx->dpi_multiply = ctx->dpi / dpi_reference;

				SDL_GetWindowSize(win, &w, &h);
				if(w < h)
					longest = w;
				else
					longest = h;

				ui_calculate_font_sizes(ctx, longest, &icon_pt, &header_pt, &regular_pt);
				font_change_pt(ctx->font, icon_pt, header_pt, regular_pt);
				ui_set_widget_sizes(ctx, longest);

				SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
					"Successfully resized interface elements by x%f",
					ctx->dpi_multiply);
			}
			break;

			case SDL_WINDOWEVENT_RESIZED:
			{
				SDL_Renderer *ren;
				SDL_Texture *new_tex;
				Uint32 texture_format;
				Sint32 new_w, new_h, longest;
				int icon_pt, header_pt, regular_pt;

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
					SDL_TEXTUREACCESS_TARGET, new_w, new_h);
				if(new_tex == NULL)
				{
					SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
						"Unable to create new texture: %s",
						SDL_GetError());
					return;
				}

				SDL_DestroyTexture(ctx->tex);
				ctx->tex = new_tex;

				if(new_w < new_h)
					longest = new_w;
				else
					longest = new_h;

				ui_calculate_font_sizes(ctx, longest,
					&icon_pt, &header_pt, &regular_pt);
				font_change_pt(ctx->font, icon_pt, header_pt, regular_pt);
				ui_set_widget_sizes(ctx, longest);

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

			if(ctx->current != this_box->ui_element)
			{
				ctx->redraw = SDL_TRUE;
				ctx->current = this_box->ui_element;
				SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
					"Selected item %p using motion",
					ctx->current);
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

			if(ctx->current != this_box->ui_element)
			{
				ctx->redraw = SDL_TRUE;
				ctx->current = this_box->ui_element;
				SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
					"Selected item %p using button",
					ctx->current);
			}

			ui_input(ctx, MENU_INSTR_EXEC_ITEM);
			SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
				"Executed item %p using button",
				ctx->current);

			break;
		}
	}

	return;
}

SDL_bool ui_should_redraw(ui_ctx_s *ctx)
{
	return ctx->redraw;
}

static void ui_draw_selection_bg(ui_ctx_s *ctx, const SDL_Rect *r)
{
	SDL_Rect outline = { .x = r->x, .y = r->y, .h = r->h, .w = r->w };

	SDL_SetRenderDrawColor(ctx->ren, 0xFF, 0xFF, 0xFF, 128);

	for(unsigned i = 0; i < 5; i++)
	{
		SDL_RenderDrawRect(ctx->ren, &outline);
		outline.x++;
		outline.y++;
		outline.h -= 2;
		outline.w -= 2;
	}

	return;
}

/**
 * Draw tile element 'el' at point 'p'.
 *
 * \param ctx	UI context.
 * \param el	UI element parameters. 
 * \param p	The top left point of the UI element to draw.
*/
static void ui_draw_tile(ui_ctx_s *ctx, ui_el_s *el, SDL_Point *p)
{
	const Uint16 len = (unsigned)(ctx->ref_tile_size);
	const SDL_Rect dim = {
		.h = len, .w = len, .x = p->x, .y = p->y
	};
	SDL_Texture *text_tex, *icon_tex;
	SDL_Rect text_dim, icon_dim;
	const SDL_Point tile_padding = { .x = 16, .y = 16 };

	/* Draw tile background. */
	SDL_SetRenderDrawColor(ctx->ren,
		el->elem.tile.bg.r, el->elem.tile.bg.g, el->elem.tile.bg.b, el->elem.tile.bg.a);
	SDL_RenderFillRect(ctx->ren, &dim);

	/* Render icon on tile. */
	icon_tex = font_render_icon(ctx->font, el->elem.tile.icon, el->elem.tile.fg);
	SDL_QueryTexture(icon_tex, NULL, NULL, &icon_dim.w, &icon_dim.h);
	icon_dim.x = p->x + (len / 2) - (icon_dim.w / 2);
	icon_dim.y = p->y + (len / 2) - (icon_dim.h / 2);

	SDL_SetTextureColorMod(icon_tex,
		el->elem.tile.fg.r, el->elem.tile.fg.g, el->elem.tile.fg.b);
	SDL_RenderCopy(ctx->ren, icon_tex, NULL, &icon_dim);
	SDL_DestroyTexture(icon_tex);

	/* Render tile label. */
	text_tex = font_render_text(ctx->font, el->elem.tile.label,
		FONT_STYLE_HEADER, FONT_QUALITY_HIGH,
		text_colour_light);
	SDL_QueryTexture(text_tex, NULL, NULL, &text_dim.w, &text_dim.h);

	switch(el->elem.tile.label_placement)
	{
	case LABEL_PLACEMENT_INSIDE_BOTTOM_LEFT:
		text_dim.x = p->x + tile_padding.x;
		text_dim.y = p->y + len - text_dim.h - tile_padding.y;
		break;

	case LABEL_PLACEMENT_INSIDE_BOTTOM_MIDDLE:
		text_dim.x = p->x + ((len - text_dim.w) / 2);
		text_dim.y = p->y + len - text_dim.h - tile_padding.y;
		break;

	case LABEL_PLACEMENT_INSIDE_BOTTOM_RIGHT:
		text_dim.x = p->x + len - text_dim.w - tile_padding.x;
		text_dim.y = p->y + len - text_dim.h - tile_padding.y;
		break;

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
	}

	/* Colour of elements within tile. */
	switch(el->elem.tile.label_placement)
	{
	case LABEL_PLACEMENT_INSIDE_BOTTOM_LEFT:
	case LABEL_PLACEMENT_INSIDE_BOTTOM_MIDDLE:
	case LABEL_PLACEMENT_INSIDE_BOTTOM_RIGHT:
		SDL_SetTextureColorMod(text_tex,
			el->elem.tile.fg.r, el->elem.tile.fg.g, el->elem.tile.fg.b);

		break;

	/* Alternate colour for text located outside of tile. */
	case LABEL_PLACEMENT_OUTSIDE_RIGHT_TOP:
	case LABEL_PLACEMENT_OUTSIDE_RIGHT_MIDDLE:
	case LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM:
	default:
		SDL_SetTextureColorMod(text_tex,
			text_colour_light.r, text_colour_light.g, text_colour_light.b);

		break;
	}

	SDL_RenderCopy(ctx->ren, text_tex, NULL, &text_dim);
	SDL_DestroyTexture(text_tex);

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
	if(ctx->current == el)
	{
		ui_draw_selection_bg(ctx, &dim);
	}

	/* Increment coordinates to next element. */
	p->y += len + tile_padding.y;
}

int ui_render_frame(ui_ctx_s *ctx)
{
	int ret = 0;
	SDL_Point vert;

	SDL_assert(ctx != NULL);
	SDL_assert(ctx->tex != NULL);

	if(ctx->redraw == SDL_FALSE)
		goto out;

	/* Initialise a new hitbox array. */
	if(ctx->hit_boxes != NULL)
	{
		sb_free(ctx->hit_boxes);
		ctx->hit_boxes = NULL;
	}

	ret = SDL_SetRenderTarget(ctx->ren, ctx->tex);
	if(ret != 0)
		goto out;

	/* Calculate where the first element should appear vertically. */
	SDL_GetRendererOutputSize(ctx->ren, &vert.x, &vert.y);
	vert.x /= 8;
	vert.y /= 16;

	SDL_SetRenderDrawColor(ctx->ren, 20, 20, 20, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->ren);

	for(ui_el_s *el = ctx->root; el->type != UI_ELEM_TYPE_END; el++)
	{
		switch(el->type)
		{
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

	ret = SDL_SetRenderTarget(ctx->ren, NULL);
	if(ret != 0)
		goto out;

	SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->ren);

	/* TODO: Do not copy full to full. */
	SDL_RenderCopy(ctx->ren, ctx->tex, NULL, NULL);

	SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "UI Rendered");
	ctx->redraw = SDL_FALSE;

out:
	return ret;
}

/**
 * Initialise user interface (UI) context when given an SDL Renderer.
*/
static ui_ctx_s *ui_init_renderer(SDL_Renderer *rend, float dpi, Uint32 format,
		ui_el_s *restrict ui_elements)
{
	int w, h;
	ui_ctx_s *ctx;
	int icon_pt, header_pt, regular_pt;

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

	ctx->root = ui_elements;
	ctx->current = ui_elements;
	ctx->redraw = SDL_TRUE;
	ctx->hit_boxes = NULL;
	ctx->dpi = dpi;
	ctx->dpi_multiply = dpi / dpi_reference;

	ui_calculate_font_sizes(ctx, h, &icon_pt, &header_pt, &regular_pt);
	ctx->font = font_init(rend, icon_pt, header_pt, regular_pt);
	if(ctx->font == NULL)
	{
		SDL_DestroyTexture(ctx->tex);
		goto err;
	}

	ui_set_widget_sizes(ctx, h);

	/* Draw the first frame. */
	ctx->redraw = SDL_TRUE;

out:
	return ctx;

err:
	SDL_free(ctx);
	ctx = NULL;
	goto out;
}

/**
 * Initialise user interface (UI) context when given an SDL Window.
*/
ui_ctx_s *ui_init(SDL_Window *win, ui_el_s *restrict ui_elements)
{
	ui_ctx_s *ctx = NULL;
	Uint32 format;
	int display_id;
	SDL_Renderer *rend;
	float dpi;

	SDL_assert(win != NULL);

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

	if(SDL_GetDisplayDPI(display_id, &dpi, NULL, NULL) != 0)
	{
		dpi = dpi_reference;
		SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
			"Unable to determine display DPI: %s", SDL_GetError());
	}

	format = SDL_GetWindowPixelFormat(win);
	ctx = ui_init_renderer(rend, dpi, format, ui_elements);

out:
	return ctx;

err:
	goto out;
}

/**
 * Free resources used by UI.
 * \param ctx UI Context
*/
void ui_exit(ui_ctx_s *ctx)
{
	font_exit(ctx->font);

	SDL_DestroyTexture(ctx->tex);
	sb_free(ctx->hit_boxes);
	SDL_free(ctx);
}
