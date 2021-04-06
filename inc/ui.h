/**
 * UI toolkit for SDL2.
 * Copyright (C) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

#include <SDL.h>

enum
{
	HAIYAJAN_LOG_CATEGORY_MAIN = SDL_LOG_CATEGORY_CUSTOM,
	HAIYAJAN_LOG_CATEGORY_UI
};

/* Private UI Context. */
typedef struct ui_ctx ui_ctx_s;

#define UI_EVENT_MASK (SDL_WINDOWEVENT | SDL_MOUSEMOTION | SDL_KEYDOWN | SDL_JOYAXISMOTION)

/* Forward decleration. */
typedef struct ui_element ui_el_s;
struct ui_element;

struct ui_tile {
	const char *label;
	enum {
		LABEL_PLACEMENT_INSIDE_BOTTOM_LEFT = 0,
		LABEL_PLACEMENT_INSIDE_BOTTOM_MIDDLE,
		LABEL_PLACEMENT_INSIDE_BOTTOM_RIGHT,
		LABEL_PLACEMENT_OUTSIDE_RIGHT_TOP,
		LABEL_PLACEMENT_OUTSIDE_RIGHT_MIDDLE,
		LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM
	} label_placement;
	enum {
		TILE_SHAPE_SQUARE = 0,
		TILE_SHAPE_CIRCLE
	} tile_shape;
	enum {
		TILE_SIZE_SMALL = 0,
		TILE_SIZE_MEDIUM,
		TILE_SIZE_LARGE,

		TILE_SIZE_MAX
	} tile_size;
	const Uint16 icon;

	const char *help;

	/* Background colour of tile. */
	SDL_Colour bg;

	/* Foreground colour of tile. The label and icon will be in this colour
	 * if they are within the tile. */
	SDL_Colour fg;

	/* If disabled, onlick events are not triggered, and the tile is
	 * faded to a dull colour. */
	SDL_bool disabled;

	enum {
		ONCLICK_GOTO_ELEMENT,
		ONCLICK_EXECUTE_FUNCTION,
		ONCLICK_SET_SIGNED_VARIABLE,
		ONCLICK_SET_UNSIGNED_VARIABLE,
	} onclick;

	union {
		struct {
			ui_el_s *element;
		} goto_element;

		struct {
			void (*function)(ui_el_s *element);
		} execute_function;

		struct {
			Sint32 *variable;
			Sint32 val;
		} signed_variable;

		struct {
			Uint32 *variable;
			Uint32 val;
		} unsigned_variable;

	} onclick_event;

	/* Pointer can be set by the user application. */
	void *user;
};

struct ui_element {
	enum {
		UI_ELEM_TYPE_END,
		UI_ELEM_TYPE_TILE
	} type;
	union {
		struct ui_tile tile;
	};
};

/**
 * Render UI to window.
*
* \param ctx	UI Context.
* \returns	SDL Texture with rendered UI.
*/
int ui_render_frame(ui_ctx_s *ctx);

/**
 * Process input and window resize events.
 *
 * \param ctx	UI Context.
 * \param e	SDL_Event that was triggered.
 */
void ui_process_event(ui_ctx_s *ctx, SDL_Event *e);

/**
 * Initialise user interface from an SDL Renderer.
 *
 * \param win	SDL_Window to target.
 * \param ui	Pointer to root (or main) menu. Must remain valid until after
 *		ui_exit is called.
 * \return	UI context. NULL on error.
 */
ui_ctx_s *ui_init(SDL_Window *win, ui_el_s *ui_elements);

SDL_bool ui_should_redraw(ui_ctx_s *ctx);

void ui_exit(ui_ctx_s *ctx);
