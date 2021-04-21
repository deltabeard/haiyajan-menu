/**
 * UI toolkit for SDL2.
 * Copyright (c) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#define XXH_INLINE_ALL

#include <font.h>
#include <SDL.h>
#include <stdint.h>
#include <stretchy_buffer.h>
#include <ui.h>

#if 1
#include <xxhash.h>

 /* Use 64 bit hashing if the target platform is also 64 bit. */
#if UINTPTR_MAX == UINT64_MAX
typedef XXH64_hash_t XXHNATIVE_hash_t;
# define XXHNATIVE(dat, len)	XXH64(dat, len, 0)
# define XXHNATIVE_FMT		"%08lx"
#else
typedef XXH32_hash_t XXHNATIVE_hash_t;
# define XXHNATIVE(dat, len) XXH32(dat, len, 0)
# define XXHNATIVE_FMT		"%04x"
#endif
#endif

static const float dpi_reference = 96.0f;
static const int dpi_ignore_thresh = 480;

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

	/**
	 * Offsets hit boxes by this number of pixels vertically.
	 */
	Uint32 touch_offset;

#if 1
	struct tex_cache
	{
		XXHNATIVE_hash_t hash;
		SDL_Texture *tex;
	} *tex_cache;
#endif

	/* DPI that tex texture is rendered for. */
	float dpi;
	float dpi_multiply;

	/* Whether the front-end must call ui_render_frame(). */
	SDL_bool redraw;

	float ref_tile_sizes[TILE_SIZE_MAX];
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
	 * Could be used when user presses BACKSPACE.
	 */
	MENU_INSTR_PARENT_MENU,

	/* Execute item operation.
	 * Could be used when user presses ENTER.
	 */
	MENU_INSTR_EXEC_ITEM
} menu_instruction_e;

#if 0
typedef enum
{
	UI_OBJECT_TYPE_TILE,
	UI_OBJECT_TYPE_PUSH_BUTTON,
	UI_OBJECT_TYPE_TOGGLE_SWITCH,
	UI_OBJECT_TYPE_DROPDOWN_LIST,
	UI_OBJECT_TYPE_SPINNER,
	UI_OBJECT_TYPE_SLIDER,
	UI_OBJECT_TYPE_HEADER
} ui_object_type_e;

struct ui_object
{
	ui_object_type_e object_type;

	/* Every object has a label that is shown with the object. */
	struct _label
	{
		SDL_bool heap;
		union _label_string
		{
			char *str_heap;
			const char *str_stack;
		} label_string;
	} label;

	/* Stack allocated help text. NULL if not required. */
	const char *help;

	union _object_parameters
	{
		struct _tile_parameters
		{
			/* Menu to change to on pressing the tile. */
			const struct menu_ctx *on_press;

			/* Background colour of the tile. */
			const SDL_Colour *bg_colour;

			/* Icon colour. */
			const SDL_Colour *icon_colour;

			/* Text colour. */
			const SDL_Colour *label_colour;

			/* Icon to show within the tile. */
			/* TODO: Set to correct type. */
			const Uint32 *icon;

			/* Size of icon. Proportional to the rest of the UI. */
			enum
			{
				SMALL, MEDIUM, LARGE
			} size;

			/* Location and alignment of tile label. */
			enum
			{
				INSIDE, OUTSIDE
			} label_location;
			enum
			{
				LEFT, MIDDLE, RIGHT
			} label_align;
		} tile;

		struct _push_button_parameters
		{
			/* Menu to change to on pressing the button. */
			struct menu_ctx *on_press;
		} push_button;

		struct _toggle_switch_parameters
		{
			/* Variable that is modified with the switch. */
			SDL_bool *val;
		} toggle_switch;

		struct _dropdown_list_parameters
		{
			/* Array of strings to show within the dropdown list. */
			const char **items;

			/* The string selected by the user. Index 0 is the first
			 * string. */
			Uint8 selected_item;
		} dropdown_list;

		struct _spinner_parameters
		{
			/* Obtains progress and the progress message for the
			 * spinner.
			 * If progress is NULL, then the spinner varies with an
			 * animation.
			 * If text is NULL, then no progress text is displayed
			 * with the spinner. */
			void (*spinner_progress)(Uint32 *progress, char **text);
		} spinner;

		struct _slider_parameters
		{
			const Uint32 min, step, max;
			Uint32 *val;
		} slider;

		struct _header_parameters
		{
			/* There are no extra options for a header object. */
			int unused;
		} header;
	} object_parameters;
};

static void ui_input(ui_ctx_s *ctx, menu_instruction_e instr)
{
	unsigned *item_selected = &ctx->current->item_selected;
	switch(instr)
	{
	case MENU_INSTR_PREV_ITEM:
		if(ctx->current->item_selected > 0)
			(*item_selected)--;
		break;

	case MENU_INSTR_NEXT_ITEM:
		if(*item_selected < (ctx->current->items.static_list.items_nmemb - 1))
			(*item_selected)++;
		break;

	case MENU_INSTR_PARENT_MENU:
		if(ctx->current->parent != NULL)
			ctx->current = ctx->current->parent;

		break;

	case MENU_INSTR_EXEC_ITEM:
	{
		const struct menu_item *item =
			ctx->current->items.static_list.items + ctx->current->item_selected;

		switch(item->op)
		{
		case MENU_SUB_MENU:
			ctx->current = item->param.sub_menu;
			break;

		case MENU_EXEC_FUNC:
			item->param.exec_func.func(item->param.exec_func.ctx);
			break;

		case MENU_SET_VAL:
			*item->param.set_val.set = item->param.set_val.val;
			break;
		}

		break;
	}
	}

	ctx->redraw = SDL_TRUE;
	return;
}

void ui_force_redraw(ui_ctx_s *ctx)
{
	ctx->redraw = SDL_TRUE;
}

SDL_bool ui_should_redraw(ui_ctx_s *ctx)
{
	return ctx->redraw;
}

int ui_render_frame(ui_ctx_s *ctx)
{
	int ret = 0;
	SDL_assert(ctx->tex != NULL);

	if(ctx->redraw == SDL_FALSE)
		goto out;

	ret = SDL_SetRenderTarget(ctx->ren, ctx->tex);
	if(ret != 0)
		goto out;

	SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->ren);

	/* Define size of each box in the main menu. */
	SDL_Rect main_menu_box = { 150, 50, MENU_BOX_DIM, MENU_BOX_DIM };
	SDL_Rect bg_box = { 145, 45, 110, 110 };

	for(unsigned item = 0;
		item < ctx->current->items.static_list.items_nmemb;
		item++)
	{
		struct menu_item *this = &ctx->current->items.static_list.items[item];
		const SDL_Colour ol = this->fg;
		const SDL_Colour bg = this->bg;
		SDL_Rect text_loc = {
			.x = main_menu_box.x + 6,
			.y = main_menu_box.y + 80,
			.h = 1, .w = 1
		};

		if(item == ctx->current->item_selected)
		{
			SDL_SetRenderDrawColor(ctx->ren, ol.r, ol.g, ol.b, ol.a);
			SDL_RenderFillRect(ctx->ren, &bg_box);
			SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "Selected %s",
				this->name);
		}

		/* Draw item box. */
		SDL_SetRenderDrawColor(ctx->ren, bg.r, bg.g, bg.b, bg.a);
		SDL_RenderFillRect(ctx->ren, &main_menu_box);

		/* Record hit-box information. */
		if(ctx->boxes_input != NULL)
		{
			ctx->boxes_input[item].hit_box.x = main_menu_box.x;
			ctx->boxes_input[item].hit_box.y = main_menu_box.y;
			ctx->boxes_input[item].hit_box.w = main_menu_box.w;
			ctx->boxes_input[item].hit_box.h = main_menu_box.h;

			ctx->boxes_input[item].item = this;
			SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "Hit box generated for (%d, %d)(%d, %d)",
				main_menu_box.x, main_menu_box.y, main_menu_box.h, main_menu_box.w);
		}

		/* Draw item text. */
		SDL_SetRenderDrawColor(ctx->ren, 0xFA, 0xFA, 0xFA, SDL_ALPHA_OPAQUE);

		/* TODO: Replace with user defined macro. */
		FontPrintToRenderer(ctx->font, this->name, &text_loc);

		main_menu_box.y += MENU_BOX_SPACING;
		bg_box.y += MENU_BOX_SPACING;
	}

	ret = SDL_SetRenderTarget(ctx->ren, NULL);
	if(ret != 0)
		goto out;

	SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->ren);

	/* TODO: Do not copy full to full. */
	SDL_RenderCopy(ctx->ren, ctx->tex, NULL, NULL);

	SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "UI Rendered %s",
		ctx->current->title);
	ctx->redraw = SDL_FALSE;

out:
	return ret;
}
#endif

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
	const float ref_tile_sizes[TILE_SIZE_MAX] = {
		60.0f, 100.0f, 160.0f
	};

	SDL_assert(ui != NULL);
	SDL_assert(ui->dpi > 0);

	if(window_height < 480)
	{
		/* Reduce size of tiles when the window size is small. */
		const float sml_p1 = 0.1146f;
		const float sml_p2 = 5.0f;
		const float med_p1 = 0.2188f;
		const float med_p2 = -5.0f;
		const float large_p1 = 0.375f;
		const float large_p2 = -20.0f;
		float x;

		x = (sml_p1 * window_height) + sml_p2;
		ui->ref_tile_sizes[TILE_SIZE_SMALL] = SDL_ceilf(x);

		x = (med_p1 * window_height) + med_p2;
		ui->ref_tile_sizes[TILE_SIZE_MEDIUM] = SDL_ceilf(x);

		x = (large_p1 * window_height) + large_p2;
		ui->ref_tile_sizes[TILE_SIZE_LARGE] = SDL_ceilf(x);
		return;
	}

	for(unsigned i = 0; i < SDL_arraysize(ui->ref_tile_sizes); i++)
	{
		ui->ref_tile_sizes[i] = ref_tile_sizes[i] * ui->dpi_multiply;
	}
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
static void ui_calculate_font_sizes(float dpi, Sint32 window_height,
	int *restrict icon_pt, int *restrict header_pt,
	int *restrict regular_pt)
{
	static const float icon_size_reference = 72.0f;
	static const float header_size_ref = 40.0f;
	static const float regular_size_ref = 24.0f;
	float dpi_multiply;

	/* Pedantic asserts for debug builds only. */
	SDL_assert(dpi > 0.0f);
	SDL_assert(window_height >= UI_MIN_WINDOW_HEIGHT);
	SDL_assert(icon_pt != NULL);
	SDL_assert(header_pt != NULL);
	SDL_assert(regular_pt != NULL);

	/* Make sure we don't abuse "restrict" keyword. */
	SDL_assert(icon_pt != header_pt);
	SDL_assert(header_pt != regular_pt);
	SDL_assert(icon_pt != regular_pt);

	/* Do not take DPI into account for low resolution displays. */
	if(window_height < dpi_ignore_thresh)
	{
		/* Curve fit model for converting window height to
		 * a suitable header font size. */
		const float p1_hdr = 0.0625f;
		const float p2_hdr = 10.0f;
		const float p1_ico = 0.1458f;
		const float p2_ico = 2.0f;
		const float p1_reg = 0.02083f;
		const float p2_reg = 14.0f;
		float pt;

		pt = (p1_hdr * (float)window_height) + p2_hdr;
		*header_pt = (int)SDL_ceilf(pt);

		pt = (p1_ico * (float)window_height) + p2_ico;
		*icon_pt = (int)SDL_ceilf(pt);

		pt = (p1_reg * (float)window_height) + p2_reg;
		*regular_pt = (int)SDL_ceilf(pt);

		return;
	}

	dpi_multiply = dpi / dpi_reference;

	*icon_pt = (int)(icon_size_reference * dpi_multiply);
	*header_pt = (int)(header_size_ref * dpi_multiply);
	*regular_pt = (int)(regular_size_ref * dpi_multiply);
}

/**
 * Clears all cached textures and hit boxes, forcing the UI toolkit to redraw
 * all widgets on next render.
 */
static void ui_clear_cache(ui_ctx_s *restrict ctx)
{
	sb_free(ctx->tex_cache);
	sb_free(ctx->hit_boxes);

	ctx->tex_cache = NULL;
	ctx->hit_boxes = NULL;

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Cleared UI cache");
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
				new_dpi = 96.0f;

			if(new_dpi == ctx->dpi)
				break;

			/* Redraw UI if window moved to a display with
			 * a different DPI. */
			ctx->dpi_multiply = ctx->dpi / dpi_reference;

			SDL_GetWindowSize(win, &w, &h);
			if(w < h)
				longest = w;
			else
				longest = h;

			ui_calculate_font_sizes(ctx->dpi, longest,
				&icon_pt, &header_pt, &regular_pt);
			font_change_pt(ctx->font, icon_pt, header_pt, regular_pt);
			ui_set_widget_sizes(ctx, longest);
			ui_clear_cache(ctx);

			SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
				"Successfully resized interface elements by x%f",
				ctx->dpi_multiply);
			ctx->redraw = SDL_TRUE;
		}
		break;

		case SDL_WINDOWEVENT_RESIZED:
		{
			SDL_Renderer *ren;
			SDL_Texture *new_tex;
			Uint32 texture_format;
			Sint32 old_w, old_h, old_longest;
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

			/* Get new size of window. */
			new_w = e->window.data1;
			new_h = e->window.data2;

			SDL_QueryTexture(ctx->tex, NULL, NULL, &old_w, &old_h);

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

			SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
				"Successfully resized texture size to %dW %dH",
				new_w, new_h);

			if(new_w < new_h)
				longest = new_w;
			else
				longest = new_h;

			if(old_w < old_h)
				old_longest = old_w;
			else
				old_longest = old_h;

			ui_clear_cache(ctx);
			ctx->redraw = SDL_TRUE;

			/* If the window has not changed size
			 * significantly enough to change the size
			 * of the elements, then do not change
			 * element sizes. */
			if(old_longest > dpi_ignore_thresh &&
				longest > dpi_ignore_thresh)
				break;

			ui_calculate_font_sizes(ctx->dpi, longest,
				&icon_pt, &header_pt, &regular_pt);
			font_change_pt(ctx->font, icon_pt, header_pt, regular_pt);
			ui_set_widget_sizes(ctx, longest);
			SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
				"Cache be cleared due to "
				"significant change in window dimensions.");
		}

		break;

		default:
			return;
		}
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
static SDL_Texture *ui_draw_tile(ui_ctx_s *ctx, ui_el_s *el, SDL_Point *p)
{
#if 0
	const unsigned len = (unsigned)(ctx->ref_tile_sizes[el->elem.tile.tile_size] * ctx->dpi_multiply);
	const SDL_Rect dim = {
		.h = len, .w = len, .x = p->x, .y = p->y
	};
	SDL_Texture *text_tex, *icon_tex, *out;
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
#endif

	/* 1. Render tile icon texture. */
	const unsigned tile_len = (unsigned)(ctx->ref_tile_sizes[el->elem.tile.tile_size] * ctx->dpi_multiply);
	SDL_Texture *text_tex, *tile, *out;
	SDL_Rect text_dim, tile_dim;
	const SDL_Point tile_padding = { .x = 16, .y = 16 };
	SDL_Rect out_sz = { 0 };
	Uint32 format;
	int r;

	r = SDL_QueryTexture(ctx->tex, &format, NULL, NULL, NULL);
	SDL_assert(r == 0);

	tile = SDL_CreateTexture(ctx->ren, format, SDL_TEXTUREACCESS_TARGET,
		tile_len, tile_len);
	SDL_assert(tile != NULL);

	tile_dim.h = tile_len;
	tile_dim.w = tile_len;
	tile_dim.x = 0;
	tile_dim.y = 0;

	r = SDL_SetRenderTarget(ctx->ren, tile);
	SDL_assert(r == 0);

	r = SDL_SetRenderDrawColor(ctx->ren,
		el->elem.tile.bg.r, el->elem.tile.bg.g, el->elem.tile.bg.b, el->elem.tile.bg.a);
	SDL_assert(r == 0);

	r = SDL_RenderClear(ctx->ren);
	SDL_assert(r == 0);

	{
		SDL_Texture *icon_tex;
		SDL_Rect icon_dim;
		icon_tex = font_render_icon(ctx->font, el->elem.tile.icon,
			el->elem.tile.fg);
		SDL_assert(icon_tex != NULL);

		r = SDL_QueryTexture(icon_tex, NULL, NULL, &icon_dim.w, &icon_dim.h);
		SDL_assert(r == 0);

		icon_dim.x = (tile_len / 2) - (icon_dim.w / 2);
		icon_dim.y = (tile_len / 2) - (icon_dim.h / 2);

		r = SDL_SetTextureColorMod(icon_tex,
			el->elem.tile.fg.r, el->elem.tile.fg.g, el->elem.tile.fg.b);
		SDL_assert(r == 0);

		r = SDL_RenderCopy(ctx->ren, icon_tex, NULL, &icon_dim);
		SDL_assert(r == 0);

		SDL_DestroyTexture(icon_tex);
	}

	/* 2. Render label texture. */
	{
		text_tex = font_render_text(ctx->font, el->elem.tile.label,
			FONT_STYLE_HEADER, FONT_QUALITY_HIGH,
			text_colour_light);
		SDL_assert(text_tex != NULL);

		r = SDL_QueryTexture(text_tex, NULL, NULL, &text_dim.w, &text_dim.h);
		SDL_assert(r == 0);

		switch(el->elem.tile.label_placement)
		{
		case LABEL_PLACEMENT_INSIDE_BOTTOM_LEFT:
			text_dim.x = tile_padding.x;
			text_dim.y = tile_len - text_dim.h - tile_padding.y;
			break;

		case LABEL_PLACEMENT_INSIDE_BOTTOM_MIDDLE:
			text_dim.x = ((tile_len - text_dim.w) / 2);
			text_dim.y = tile_len - text_dim.h - tile_padding.y;
			break;

		case LABEL_PLACEMENT_INSIDE_BOTTOM_RIGHT:
			text_dim.x = tile_len - text_dim.w - tile_padding.x;
			text_dim.y = tile_len - text_dim.h - tile_padding.y;
			break;

		case LABEL_PLACEMENT_OUTSIDE_RIGHT_TOP:
			text_dim.x = tile_len + tile_padding.x;
			text_dim.y = 0;
			break;

		case LABEL_PLACEMENT_OUTSIDE_RIGHT_MIDDLE:
			text_dim.x = tile_len + tile_padding.x;
			text_dim.y = (tile_len / 2) - (text_dim.h / 2);
			break;

		case LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM:
			text_dim.x = tile_len + tile_padding.x;
			text_dim.y = tile_len - text_dim.h;
			break;
		}
	}

	/* 3. Draw textures to entry target. */
	{
		out_sz.h = tile_len;
		out_sz.w = text_dim.w + tile_len + tile_padding.x;
		out = SDL_CreateTexture(ctx->ren, format,
			SDL_TEXTUREACCESS_TARGET, out_sz.w, out_sz.h);
		SDL_assert(out != NULL);

		r = SDL_SetRenderTarget(ctx->ren, out);
		SDL_assert(r == 0);

		/* Make texture background transparent. */
		/* This probably isn't required. */
		r = SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
		SDL_assert(r == 0);
		r = SDL_RenderClear(ctx->ren);
		SDL_assert(r == 0);

		/* Use alpha blending on texture. */
		r = SDL_SetTextureBlendMode(out, SDL_BLENDMODE_BLEND);
		SDL_assert(r == 0);

		r = SDL_RenderCopy(ctx->ren, tile, NULL, &tile_dim);
		SDL_assert(r == 0);
		r = SDL_RenderCopy(ctx->ren, text_tex, NULL, &text_dim);
		SDL_assert(r == 0);
	}

	return out;
}

int ui_render_frame(ui_ctx_s *ctx)
{
	int ret = 0;
	SDL_Point vert;

	SDL_assert(ctx != NULL);
	SDL_assert(ctx->tex != NULL);

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
		struct tex_cache new_cache_entry;
		XXHNATIVE_hash_t h = XXHNATIVE(el, sizeof(ui_el_s));
		unsigned tcache_n = sb_count(ctx->tex_cache);
		SDL_Rect targ;
		SDL_Texture *el_tex;

		/* Search texture cache for rendered widget. */
		for(unsigned i = 0; i < tcache_n; i++)
		{
			struct tex_cache *tc = &ctx->tex_cache[i];
			int tw, th;

			/* Move to next texture if hash check fails. */
			if(tc->hash != h)
				continue;

			el_tex = tc->tex;
			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
				"Cached texture " XXHNATIVE_FMT " used for element %p",
				h, el);

			/* Skip rendering a new texture. */
			goto next_element;
		}

		/* If we reach here, it means that there is no rendered texture
		 * in the texture cache. */

		switch(el->type)
		{
		case UI_ELEM_TYPE_TILE:
			new_cache_entry.tex = ui_draw_tile(ctx, el, &vert);
			break;

		default:
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"The requested UI element %d is not implemented.",
				el->type);
			SDL_assert_always(0);
			break;
		}

		new_cache_entry.hash = h;
		sb_push(ctx->tex_cache, new_cache_entry);
		el_tex = new_cache_entry.tex;

		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
			"Redrew texture " XXHNATIVE_FMT " for element %p",
			h, el);

next_element:
		targ.x = vert.x;
		targ.y = vert.y;

		SDL_SetRenderTarget(ctx->ren, ctx->tex);
		SDL_QueryTexture(el_tex, NULL, NULL, &targ.w, &targ.h);
		SDL_RenderCopy(ctx->ren, el_tex, NULL, &targ);

		vert.y += targ.h;

		continue;
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

	ui_calculate_font_sizes(dpi, h, &icon_pt, &header_pt, &regular_pt);
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
		dpi = 96.0f;
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
