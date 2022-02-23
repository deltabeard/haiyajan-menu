/**
 * UI toolkit for SDL2.
 * Copyright (C) 2020-2022 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

#include <font.h>
#include <hedley.h>
#include <SDL.h>

#define UI_MIN_WINDOW_WIDTH	160
#define UI_MIN_WINDOW_HEIGHT	144
#define UI_DEFAULT_WINDOW_WIDTH 640
#define UI_DEFAULT_WINDOW_HEIGHT 480

enum {
	HAIYAJAN_LOG_CATEGORY_MAIN = SDL_LOG_CATEGORY_CUSTOM,
	HAIYAJAN_LOG_CATEGORY_UI
};

/* Private UI Context. */
typedef struct ui_ctx ui_ctx_s;

#define UI_EVENT_MASK (SDL_WINDOWEVENT | SDL_MOUSEMOTION | SDL_KEYDOWN | SDL_JOYAXISMOTION)

/* Forward declerations. */
struct ui_element;

/* Enumurators. */
typedef enum
{
	LABEL_PLACEMENT_OUTSIDE_RIGHT_TOP = 0,
	LABEL_PLACEMENT_OUTSIDE_RIGHT_MIDDLE,
	LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,

	LABEL_PLACEMENT_MAX
} label_placement_e;

typedef enum
{
	UI_EVENT_NOP,
	UI_EVENT_GOTO_ELEMENT,
	UI_EVENT_EXECUTE_FUNCTION,
	UI_EVENT_SET_SIGNED_VARIABLE,
	UI_EVENT_SET_UNSIGNED_VARIABLE,
} ui_event_action_e;

typedef enum
{
	UI_ELEM_TYPE_END,
	UI_ELEM_TYPE_LABEL,
	UI_ELEM_TYPE_TILE,
	UI_ELEM_TYPE_DYNAMIC,
	UI_ELEM_TYPE_BAR
} ui_elem_type_e;

/* Structures. */
struct ui_event
{
	ui_event_action_e action;

	union
	{
		struct
		{
			const struct ui_element *element;
		} goto_element;

		struct
		{
			void (*function)(const struct ui_element *element);
		} execute_function;

		struct
		{
			Sint32 *variable;
			Sint32 val;
		} signed_variable;

		struct
		{
			Uint32 *variable;
			Uint32 val;
		} unsigned_variable;

	} action_data;
};

/**
 * A tile element.
 */
struct ui_tile {
	/* Placement of label on the tile. This may affect the font colour. */
	label_placement_e label_placement;

	/* Icon in UTF-16 */
	const Uint16 icon;

	/* Help text to display when the element is highlighted.
	 * May be set to NULL if no help is available. */
	const char *help;

	/* Background colour of tile. */
	SDL_Colour bg;

	/* Foreground colour of tile. The label and icon will be in this colour
	 * if they are within the tile. */
	SDL_Colour fg;

	/* If disabled, onlick events are not triggered, and the tile is
	 * faded to a dull colour. */
	SDL_bool disabled;

	/* What to do on click. */
	struct ui_event onclick;

	/* Pointer can be set by the user application. */
	void *user;
};

/**
 * Displays a label.
 * Not selectable by the user.
 */
struct ui_label
{
	font_style_e style;
};

struct ui_bar
{
	/* Bar fill between 0 to SDL_MAX_UINT16. */
	Uint16 value;
};

struct ui_dynamic
{
	/* Returns the number of elements in menu.
	 * Returning 0 hides this menu. */
	unsigned (*number_of_elements)(void *user_ctx);

	/* Fill array elements with nmemb number of elements. Returns number
	 * of elements actually stored within input buffer.
	 * The last element is always UI_ELEM_TYPE_END. If nmemb is 1, then
	 * the only element stored in the buffer is UI_ELEM_TYPE_END.
	 * Returning 0 hides this menu. */
	unsigned (*get_elements)(unsigned *nmemb,
		struct ui_element *elements, void *user_ctx);

	/* Cached ui_elements array. Must be set to NULL in initialisation. */
	struct ui_element *cache;

	/* User context that is passed to the above functions (optional). */
	void *user_ctx;
};

struct ui_element {
	ui_elem_type_e type;

	/* Label associated with the element. */
	const char *label;

	union {
		struct ui_dynamic dynamic;
		struct ui_label label;
		struct ui_tile tile;
		struct ui_bar bar;
	} elem;
};

/**
 * Initialise user interface from an SDL Renderer.
 *
 * \param win	SDL_Window to target.
 * \param ui	Pointer to root (or main) menu. Must remain valid until after
 *		ui_exit is called.
 * \return	UI context. NULL on error.
 */
ui_ctx_s *ui_init(SDL_Window *HEDLEY_RESTRICT win,
	const struct ui_element *HEDLEY_RESTRICT ui_elements);

/**
 * Render UI to window.
 *
 * \param ctx	UI Context.
 * \returns	SDL Texture with rendered UI.
 */
SDL_Texture *ui_render_frame(ui_ctx_s *ctx);

/**
 * Process input and window resize events.
 *
 * \param ctx	UI Context.
 * \param e	SDL_Event that was triggered.
 */
void ui_process_event(ui_ctx_s *HEDLEY_RESTRICT ctx, SDL_Event *HEDLEY_RESTRICT e);

/**
 * Free UI context.
 *
 * \param ctx	UI Context.
 */
void ui_exit(ui_ctx_s *ctx);
