/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#define DONT_HAVE_FRIBIDI_CONFIG_H
#define FRIBIDI_NO_DEPRECATED

#include <fribidi.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <ui.h>

#include <fonts/NotoSansDisplay-Regular-Latin.ttf.h>
#include <fonts/NotoSansDisplay-SemiCondensedLight-Latin.ttf.h>
#include <fonts/fabric-icons.ttf.h>

static const float dpi_reference = 96.0f;
static const SDL_Colour text_colour_light = { 0xFF, 0xFF, 0xFF, SDL_ALPHA_TRANSPARENT };

struct ui_element_cache
{
	SDL_Texture *label;
	SDL_Texture *icon;
};

struct ui_ctx
{
	/* Required to recreate texture on resizing. */
	SDL_Renderer *ren;

	/* Texture to render on. */
	SDL_Texture *tex;

	/* Root Menu. */
	ui_e *root;

	/* Currently rendered menu. */
	ui_e *current;

	/* Font context used to draw text on UI elements. */
	struct
	{
		TTF_Font *title;
		TTF_Font *normal;
		TTF_Font *large_icons;
	} fonts;

	/* Rendered input boxes for touch and mouse input. */
	struct boxes_input
	{
		/* Dimensions of hit-box on screen. */
		SDL_Rect hit_box;

		/* UI element associated with hit-box. */
		ui_e *ui_element;
	} *boxes_input;

	unsigned tile_sizes[3];

	/* DPI that tex texture is rendered for. */
	float dpi;
	float dpi_multiply;

	/* Whether the front-end must call ui_render_frame(). */
	SDL_bool redraw;
};

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
	else if(e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_RESIZED)
	{
		SDL_Texture *new_tex;
		SDL_Window *win;
		SDL_Renderer *ren;
		Uint32 texture_format;
		Sint32 new_w, new_h;

		win = SDL_GetWindowFromID(e->window.windowID);
		if(win == NULL)
		{
			SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO,
				"Unable to obtain window from ID %d: %s",
				e->window.windowID, SDL_GetError());
			return;
		}

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

		SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
			"Successfully resized texture size to %dW %dH",
			new_w, new_h);

		ctx->redraw = SDL_TRUE;
	}
	else if(e->type == SDL_MOUSEMOTION && ctx->boxes_input != NULL)
	{
		SDL_Point p = {
			.x = e->motion.x,
			.y = e->motion.y
		};

		for(Uint32 box = 0;
			box < ctx->current->items.static_list.items_nmemb;
			box++)
		{
			struct boxes_input *this_box = &ctx->boxes_input[box];
			const SDL_Rect *r = &this_box->hit_box;
			SDL_bool intersects = SDL_PointInRect(&p, r);

			if(intersects == SDL_FALSE)
				continue;

			if(ctx->current->item_selected != box)
			{
				ctx->redraw = SDL_TRUE;
				ctx->current->item_selected = box;
				SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
					"Selected item %u using motion",
					ctx->current->item_selected);
			}

			break;
		}
	}
	else if(e->type == SDL_MOUSEBUTTONUP && ctx->boxes_input != NULL)
	{
		SDL_Point p = {
			.x = e->button.x,
			.y = e->button.y
		};

		if(e->button.button != SDL_BUTTON_LEFT)
			return;

		if(e->button.clicks == 0)
			return;

		for(Uint32 box = 0;
			box < ctx->current->items.static_list.items_nmemb;
			box++)
		{
			struct boxes_input *this_box = &ctx->boxes_input[box];
			const SDL_Rect *r = &this_box->hit_box;
			SDL_bool intersects = SDL_PointInRect(&p, r);

			if(intersects == SDL_FALSE)
				continue;

			if(ctx->current->item_selected != box)
			{
				ctx->redraw = SDL_TRUE;
				ctx->current->item_selected = box;
				SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
					"Selected item %d using button",
					ctx->current->item_selected);
			}

			ui_input(ctx, MENU_INSTR_EXEC_ITEM);
			SDL_LogDebug(SDL_LOG_CATEGORY_INPUT,
				"Executed item %d using button",
				ctx->current->item_selected);

			break;
		}
	}

	return;
}
#endif

SDL_bool ui_should_redraw(ui_ctx_s *ctx)
{
	return ctx->redraw;
}

int ui_render_frame(ui_ctx_s *ctx)
{
	int ret = 0;
	SDL_Point vert;

	SDL_assert(ctx != NULL);
	SDL_assert(ctx->tex != NULL);

	if(ctx->redraw == SDL_FALSE)
		goto out;

	ret = SDL_SetRenderTarget(ctx->ren, ctx->tex);
	if(ret != 0)
		goto out;

	/* Calculate where the first element should appear vertically. */
	SDL_GetRendererOutputSize(ctx->ren, &vert.x, &vert.y);
	vert.x /= 8;
	vert.y /= 16;

	SDL_SetRenderDrawColor(ctx->ren, 20, 20, 20, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->ren);

	for(ui_e *u = ctx->root; u->type != UI_ELEM_TYPE_END; u++)
	{
		/* Assuming elements are always tiles. */
		const unsigned len = ctx->tile_sizes[u->tile.tile_size];
		const SDL_Rect dim = {
			.h = len, .w = len, .x = vert.x, .y = vert.y
		};
		SDL_Surface *text_surf, *icon_surf;
		SDL_Texture *text_tex, *icon_tex;
		SDL_Rect text_dim, icon_dim;
		const SDL_Point tile_padding = { .x = 8, .y = 16 };

		/* Draw tile background. */
		SDL_SetRenderDrawColor(ctx->ren,
			u->tile.bg.r, u->tile.bg.g, u->tile.bg.b, u->tile.bg.a);
		SDL_RenderFillRect(ctx->ren, &dim);

		/* Render icon on tile. */
		icon_surf = TTF_RenderGlyph_Blended(ctx->fonts.large_icons,
			u->tile.icon, u->tile.fg);
		SDL_assert(icon_surf != NULL);
		icon_tex = SDL_CreateTextureFromSurface(ctx->ren, icon_surf);
		SDL_assert(icon_tex != NULL);
		icon_dim.w = icon_surf->w;
		icon_dim.h = icon_surf->h;
		icon_dim.x = vert.x + (len / 2) - (icon_dim.w / 2);
		icon_dim.y = vert.y + (len / 2) - (icon_dim.h / 2);

		SDL_SetTextureColorMod(icon_tex,
			u->tile.fg.r, u->tile.fg.g, u->tile.fg.b);
		SDL_RenderCopy(ctx->ren, icon_tex, NULL, &icon_dim);
		SDL_DestroyTexture(icon_tex);
		SDL_FreeSurface(icon_surf);

		{
			/* Render text on tile. */
			size_t instrlen = SDL_strlen(u->tile.label);
			FriBidiChar *instr, *outstr;
			FriBidiParType biditype = FRIBIDI_PAR_ON;
			FriBidiStrIndex strinlen;
			char *strutf8_out;

			instr = SDL_malloc(instrlen * sizeof(FriBidiChar));
			outstr = SDL_malloc(instrlen * sizeof(FriBidiChar));
			strinlen = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, u->tile.label, instrlen, instr);

			fribidi_log2vis(instr, strinlen, &biditype, outstr, NULL, NULL, NULL);
			SDL_free(instr);

			strutf8_out = SDL_malloc(instrlen);
			fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, outstr, strinlen, strutf8_out);
			SDL_free(outstr);

			text_surf = TTF_RenderUTF8_Blended(ctx->fonts.title,
				strutf8_out, text_colour_light);
			SDL_assert(text_surf != NULL);
			SDL_free(strutf8_out);
		}

		/* Render icon text. */
		text_tex = SDL_CreateTextureFromSurface(ctx->ren, text_surf);
		SDL_assert(text_tex != NULL);

		text_dim.w = text_surf->w;
		text_dim.h = text_surf->h;
		switch(u->tile.label_placement)
		{
		case LABEL_PLACEMENT_INSIDE_BOTTOM_LEFT:
			text_dim.x = vert.x + tile_padding.x;
			text_dim.y = vert.y + len - text_surf->h - tile_padding.y;
			break;

		case LABEL_PLACEMENT_INSIDE_BOTTOM_MIDDLE:
			text_dim.x = vert.x + ((len - text_surf->w) / 2);
			text_dim.y = vert.y + len - text_surf->h - tile_padding.y;
			break;

		case LABEL_PLACEMENT_INSIDE_BOTTOM_RIGHT:
			text_dim.x = vert.x + len - text_surf->w - tile_padding.x;
			text_dim.y = vert.y + len - text_surf->h - tile_padding.y;
			break;

		case LABEL_PLACEMENT_OUTSIDE_RIGHT_TOP:
			text_dim.x = vert.x + len + tile_padding.x;
			text_dim.y = vert.y;
			break;

		case LABEL_PLACEMENT_OUTSIDE_RIGHT_MIDDLE:
			text_dim.x = vert.x + len + tile_padding.x;
			text_dim.y = vert.y + (len / 2) - (text_surf->h / 2);
			break;

		case LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM:
			text_dim.x = vert.x + len + tile_padding.x;
			text_dim.y = vert.y + len - text_surf->h;
			break;
		}

		/* Colour of elements within tile. */
		switch(u->tile.label_placement)
		{
		case LABEL_PLACEMENT_INSIDE_BOTTOM_LEFT:
		case LABEL_PLACEMENT_INSIDE_BOTTOM_MIDDLE:
		case LABEL_PLACEMENT_INSIDE_BOTTOM_RIGHT:
			SDL_SetTextureColorMod(text_tex,
				u->tile.fg.r, u->tile.fg.g, u->tile.fg.b);

			break;

		default:
			SDL_SetTextureColorMod(text_tex,
				text_colour_light.r, text_colour_light.g, text_colour_light.b);

			break;
		}

		SDL_RenderCopy(ctx->ren, text_tex, NULL, &text_dim);
		SDL_DestroyTexture(text_tex);
		SDL_FreeSurface(text_surf);

		/* Increment coordinates to next element. */
		vert.y += len + tile_padding.y;
	}

#if 0
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
#endif

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

static int ui_init_fonts(ui_ctx_s *ctx, float dpi)
{
	const int title_pt_ref = 28, normal_pt_ref = 22;
	int ret = 1;
	size_t datasize;
	void *ui_ttf;
	ui_ttf = SDL_LoadFile("C:\\Windows\\Fonts\\arial.ttf", &datasize);
	//ui_ttf = SDL_LoadFile("C:\\Windows\\Fonts\\YuGothR.ttc", &datasize);
	//ui_ttf = SDL_LoadFile("C:\\Windows\\Fonts\\segoeuil.ttf", &datasize);

	struct font_info
	{
		int pt_ref;
		TTF_Font **ttf;
		const unsigned char *font_mem;
		const int font_len;
	} const font_infos[] = {
		{
			38, &ctx->fonts.title,
			ui_ttf,
			datasize
		},
		{
			24, &ctx->fonts.normal,
			NotoSansDisplay_Regular_Latin_ttf,
			NotoSansDisplay_Regular_Latin_ttf_len
		},
		{
			72, &ctx->fonts.large_icons,
			fabric_icons_ttf,
			fabric_icons_ttf_len
		}
	};

	for(unsigned i = 0; i < SDL_arraysize(font_infos); i++)
	{
		const struct font_info *f = &(font_infos[i]);
		TTF_Font **ttf = f->ttf;
		SDL_RWops *font_mem;
		float pt;

		font_mem = SDL_RWFromConstMem(f->font_mem, f->font_len);
		if(font_mem == NULL)
			goto err;

		if(*ttf != NULL)
			TTF_CloseFont(*ttf);

		pt = f->pt_ref * ctx->dpi_multiply;
		*ttf = TTF_OpenFontRW(font_mem, 1, pt);
	}

	ret = 0;

err:
	return ret;
}

static void ui_recalculate_element_sizes(ui_ctx_s *ctx)
{
	const float ref_tile_sizes[] = {
		60.0f, 100.0f, 160.0f
	};
	ctx->tile_sizes[TILE_SIZE_SMALL] = ref_tile_sizes[TILE_SIZE_SMALL] * ctx->dpi_multiply;
	ctx->tile_sizes[TILE_SIZE_MEDIUM] = ref_tile_sizes[TILE_SIZE_MEDIUM] * ctx->dpi_multiply;
	ctx->tile_sizes[TILE_SIZE_LARGE] = ref_tile_sizes[TILE_SIZE_LARGE] * ctx->dpi_multiply;
	return;
}

/**
 * Initialise user interface (UI) context when given an SDL Renderer.
*/
static ui_ctx_s *ui_init_renderer(SDL_Renderer *rend, float dpi, Uint32 format,
	ui_e *ui_elements)
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

	ctx->root = ui_elements;
	ctx->current = ui_elements;
	ctx->redraw = SDL_TRUE;
	ctx->boxes_input = NULL;
	ctx->dpi = dpi;
	ctx->dpi_multiply = dpi / dpi_reference;

	if(ui_init_fonts(ctx, ctx->dpi) != 0)
	{
		SDL_DestroyTexture(ctx->tex);
		goto err;
	}

	ui_recalculate_element_sizes(ctx);

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
ui_ctx_s *ui_init(SDL_Window *win, ui_e *ui_elements)
{
	ui_ctx_s *ctx = NULL;
	Uint32 format;
	int display_id;
	SDL_Renderer *rend;
	float dpi;

	if(!TTF_WasInit() && TTF_Init() == -1)
	{
		SDL_SetError("TTF_Init: %s", TTF_GetError());
		goto err;
	}

	SDL_assert_paranoid(win != NULL);

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
	TTF_CloseFont(ctx->fonts.title);
	TTF_CloseFont(ctx->fonts.normal);
	TTF_CloseFont(ctx->fonts.large_icons);
	TTF_Quit();

	SDL_DestroyTexture(ctx->tex);
	SDL_free(ctx->boxes_input);
	SDL_free(ctx);
}
