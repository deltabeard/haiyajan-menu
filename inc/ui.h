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

int ui_redraw(struct ui_s *ui, SDL_Renderer *rend);
void ui_input(struct ui_s *ui, SDL_GameControllerButton btn);
ui_ctx *ui_init(SDL_Renderer *ren, Uint32 texture_format);
