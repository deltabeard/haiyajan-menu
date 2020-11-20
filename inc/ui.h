#pragma once

#include <font.h>
#include <menu.h>
#include <SDL.h>

/* Private UI Context. */
typedef struct ui_ctx ui_ctx;

/* Configurable UI settings. */
struct ui_entry_style {
	SDL_Colour bg;
	SDL_Colour selected_outline;
};

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
struct ui_ctx *ui_init(SDL_Window *win, struct menu_ctx *root, font_ctx *font);
SDL_bool ui_should_redraw(ui_ctx *c);
