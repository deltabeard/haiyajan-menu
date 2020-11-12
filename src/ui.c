/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <menu.h>
#include <SDL.h>
#include <ui.h>

struct ui_ctx
{
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
};

void ui_input(ui_ctx *ui, SDL_GameControllerButton btn)
{
	switch(btn)
	{
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		ui->current = menu_instruct(ui->current, MENU_INSTR_PREV_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		ui->current = menu_instruct(ui->current, MENU_INSTR_NEXT_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_A:
		ui->current = menu_instruct(ui->current, MENU_INSTR_EXEC_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_B:
		ui->current = menu_instruct(ui->current, MENU_INSTR_PARENT_MENU);
		break;

	default:
		return;
	}

	SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Redraw requested");
	ui->redraw = SDL_TRUE;
	return;
}

SDL_bool ui_should_redraw(ui_ctx *c)
{
	return c->redraw;
}

int ui_render_frame(ui_ctx *c)
{
	int ret = 0;
	SDL_assert(c->tex != NULL);

	if(c->redraw == SDL_FALSE)
		goto out;

	ret = SDL_SetRenderTarget(c->ren, c->tex);

	SDL_SetRenderDrawColor(c->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(c->ren);

	SDL_Rect main_menu_box = {150, 50, 100, 100};
	const unsigned box_spacing = 120;

	for(unsigned item = 0; item < c->current->items_u.static_list.items_nmemb; item++)
	{
		const SDL_Colour ol = c->current->items_u.static_list.items[item].selected_outline;
		const SDL_Colour bg = c->current->items_u.static_list.items[item].bg;

		if(item == c->current->item_selected)
		{
			SDL_Rect bg_box = main_menu_box;
			SDL_SetRenderDrawColor(c->ren, ol.r, ol.g, ol.b, ol.a);
			SDL_RenderDrawRect(c->ren, &bg_box);
			SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Selected %s",
				    c->current->items_u.static_list.items[item].name);
		}

		SDL_SetRenderDrawColor(c->ren, bg.r, bg.g, bg.b, bg.a);
		SDL_RenderFillRect(c->ren, &main_menu_box);
		main_menu_box.y += box_spacing;
	}

	ret = SDL_SetRenderTarget(c->ren, NULL);
	SDL_RenderCopy(c->ren, c->tex, NULL, NULL);

	SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "UI Rendered %s",
		    c->current->title);
	c->redraw = SDL_FALSE;

out:
	return ret;
}

void ui_process_event(ui_ctx *c, SDL_Event *e)
{
	/* Recalculate begin_actual coordinates on resolution and DPI change. */
	return;
}

struct ui_ctx *ui_init(SDL_Window *win, SDL_Renderer *ren, struct menu_ctx *root)
{
	int w, h;
	ui_ctx *c;
	Uint32 texture_format;
	int display_id;

	SDL_assert_paranoid(win != NULL);
	SDL_assert_paranoid(ren != NULL);

	c = SDL_calloc(1, sizeof(ui_ctx));
	if(c == NULL)
		goto err;

	if(SDL_GetRendererOutputSize(ren, &w, &h) != 0)
		goto err;

	display_id = SDL_GetWindowDisplayIndex(win);
	if(display_id < 0)
		goto err;

	if(SDL_GetDisplayDPI(display_id, &c->dpi, NULL, NULL) != 0)
		goto err;

	/* We keep the Renderer so that we can create textures in the
	future. */
	c->ren = ren;

	texture_format = SDL_GetWindowPixelFormat(win);
	c->tex = SDL_CreateTexture(ren, texture_format,
				   SDL_TEXTUREACCESS_TARGET, w, h);
	if(c->tex == NULL)
		goto err;

	c->root = root;
	c->current = root;
	c->redraw = SDL_TRUE;

out:
	return c;

err:
	SDL_free(c);
	c = NULL;
	goto out;
}

void ui_exit(ui_ctx *c)
{
	SDL_free(c);
}
