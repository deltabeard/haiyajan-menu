#pragma once

#include <font.h>
#include <menu.h>
#include <SDL.h>

/* Private UI Context. */
typedef struct ui_ctx ui_ctx_s;

#define UI_EVENT_MASK (SDL_WINDOWEVENT)

/* UI configuration for item. Must be provided to menu entry priv pointer. */
struct item_priv {
	const SDL_Colour bg;
	const SDL_Colour fg;
};

/**
 * Render UI to texture.
*
* \param c	UI Context.
* \returns	SDL Texture with rendered UI.
*/
int ui_render_frame(ui_ctx_s *ctx);
void ui_process_event(ui_ctx_s *ctx, SDL_Event *e);
void ui_input(ui_ctx_s *ctx, SDL_GameControllerButton btn);
ui_ctx_s *ui_init_renderer(SDL_Renderer *rend, float dpi, Uint32 format, struct menu_ctx *root, font_ctx *font);
ui_ctx_s *ui_init(SDL_Window *win, struct menu_ctx *root, font_ctx *font);
SDL_bool ui_should_redraw(ui_ctx_s *ctx);
