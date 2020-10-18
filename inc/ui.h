#pragma once

#include <menu.h>
#include <SDL.h>

struct ui_s
{
	menu_ctx root;
	menu_ctx *current;
	SDL_bool redraw_required;
	int cont;
};

/* Private UI Context. */
typedef struct ui_ctx_s ui_ctx;

/* Coordinates relative to SDL_MAX_UINT32. */
typedef struct rel_rect
{
	Uint32 x, y;
	Uint32 w, h;
} rel_rect;

/**
 * Render UI to texture.
*
* \param c	UI Context.
* \returns	SDL Texture with rendered UI.
*/
SDL_Texture *ui_render_frame(ui_ctx *c);
void ui_input(struct ui_s *ui, SDL_GameControllerButton btn);
ui_ctx *ui_init(SDL_Window *win, SDL_Renderer *ren);
