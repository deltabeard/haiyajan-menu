#pragma once

#include <font.h>
#include <menu.h>
#include <SDL.h>

/* Private UI Context. */
typedef struct ui_ctx_s ui_ctx;

#define UI_EVENT_MASK (SDL_WINDOWEVENT)

/**
 * Render UI to texture.
*
* \param c	UI Context.
* \returns	SDL Texture with rendered UI.
*/
int ui_render_frame(ui_ctx *c);
void ui_process_event(ui_ctx *c, SDL_Event *e);
void ui_input(ui_ctx *ui, SDL_GameControllerButton btn);
ui_ctx *ui_init_renderer(SDL_Renderer *rend, float dpi, Uint32 format, struct menu_ctx *root, font_ctx *font);
ui_ctx *ui_init(SDL_Window *win, struct menu_ctx *root, font_ctx *font);
void ui_force_redraw(ui_ctx *c);
SDL_bool ui_should_redraw(ui_ctx *c);
