/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <font.h>
#include <menu.h>
#include <SDL.h>
#include <ui.h>

#define MENU_BOX_DIM 100
#define MENU_BOX_SPACING 120

struct ui_ctx {
	/* Required to recreate texture on resizing. */
	SDL_Renderer *ren;

	/* Texture to render on. */
	SDL_Texture *tex;

	/* Whether the front-end must call ui_render_frame(). */
	SDL_bool redraw;

	/* Root Menu. */
	struct menu_ctx *root;
	/* Currently rendered menu. */
	struct menu_ctx *current;

	/* DPI that tex texture is rendered for. */
	float dpi;

	/* Font context used to draw text on UI elements. */
	font_ctx *font;

	/* Rendered input boxes for touch and mouse input. */
	struct boxes_input {
		/* Dimensions of hit-box on screen. */
		SDL_Rect hit_box;

		/* Menu item associated with hit-box. */
		struct menu_item *item;
	} *boxes_input;
};

void ui_input(ui_ctx_s *ctx, SDL_GameControllerButton btn)
{
	switch(btn)
	{
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		ctx->current = menu_instruct(ctx->current, MENU_INSTR_PREV_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		ctx->current = menu_instruct(ctx->current, MENU_INSTR_NEXT_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_A:
		ctx->current = menu_instruct(ctx->current, MENU_INSTR_EXEC_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_B:
		ctx->current = menu_instruct(ctx->current, MENU_INSTR_PARENT_MENU);
		break;

	default:
		return;
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

	SDL_SetRenderDrawColor(ctx->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ctx->ren);

	/* Define size of each box in the main menu. */
	SDL_Rect main_menu_box = {150, 50, MENU_BOX_DIM, MENU_BOX_DIM};
	SDL_Rect bg_box = {145, 45, 110, 110};

	for(unsigned item = 0;
		item < ctx->current->items.static_list.items_nmemb;
		item++)
	{
		struct menu_item *this_item = &ctx->current->items.static_list.items[item];
		struct item_priv *style = this_item->priv;
		const SDL_Colour ol = style->fg;
		const SDL_Colour bg = style->bg;
		SDL_Rect text_loc = {
			.x = main_menu_box.x + 6,
			.y = main_menu_box.y + 80,
			.h = 1, .w = 1
		};

		if(item == ctx->current->item_selected)
		{
			SDL_SetRenderDrawColor(ctx->ren, ol.r, ol.g, ol.b, ol.a);
			SDL_RenderFillRect(ctx->ren, &bg_box);
			SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Selected %s",
				this_item->name);
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

			ctx->boxes_input[item].item = this_item;
			SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "Hit box generated for (%d, %d)(%d, %d)",
				main_menu_box.x, main_menu_box.y, main_menu_box.h, main_menu_box.w);
		}

		/* Draw item text. */
		SDL_SetRenderDrawColor(ctx->ren, 0xFA, 0xFA, 0xFA, SDL_ALPHA_OPAQUE);
		FontPrintToRenderer(ctx->font, this_item->name, &text_loc);
		main_menu_box.y += MENU_BOX_SPACING;
		bg_box.y += MENU_BOX_SPACING;
	}

	ret = SDL_SetRenderTarget(ctx->ren, NULL);
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
	if(e->type == SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_RESIZED)
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
					"Selected item %d using motion",
					ctx->current->item_selected);
			}

			break;
		}
	}

	return;
}

ui_ctx_s *ui_init_renderer(SDL_Renderer *rend, float dpi, Uint32 format,
	struct menu_ctx *root, font_ctx *font)
{
	int w, h;
	ui_ctx_s *ctx;

	/* TODO: Create texture size limited by number of menu entries. */
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

	ctx->root = root;
	ctx->current = root;
	ctx->redraw = SDL_TRUE;
	ctx->font = font;
	ctx->boxes_input = SDL_calloc(root->items.static_list.items_nmemb,
		sizeof(*ctx->boxes_input));

	if(ctx->boxes_input == NULL)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_INPUT,
			"Unable to initialise memory for hit-box recording: %s. "
			"Mouse motion input will not be available.",
			SDL_GetError());
	}

out:
	return ctx;

err:
	SDL_free(ctx);
	ctx = NULL;
	goto out;
}

ui_ctx_s *ui_init(SDL_Window *win, struct menu_ctx *root, font_ctx *font)
{
	ui_ctx_s *ctx = NULL;
	Uint32 format;
	int display_id;
	SDL_Renderer *rend;
	float dpi;

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
	ctx = ui_init_renderer(rend, dpi, format, root, font);

out:
	return ctx;

err:
	goto out;
}

void ui_exit(ui_ctx_s *ctx)
{
	SDL_free(ctx->boxes_input);
	SDL_free(ctx);
	ctx = NULL;
}
