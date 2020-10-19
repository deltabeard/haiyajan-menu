#pragma once

#include <menu.h>
#include <SDL.h>

/* Private UI Context. */
typedef struct ui_ctx ui_ctx;

/**
 * Render UI to texture.
*
* \param c	UI Context.
* \returns	SDL Texture with rendered UI.
*/
int ui_render_frame(ui_ctx *c);
void ui_input(ui_ctx *ui, SDL_GameControllerButton btn);
struct ui_ctx *ui_init(SDL_Window *win, SDL_Renderer *ren, struct menu_ctx *root);
SDL_bool ui_should_redraw(ui_ctx *c);
