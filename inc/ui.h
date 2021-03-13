/**
 * UI toolkit for SDL2.
 * Copyright (C) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

#include <font.h>
#include <SDL.h>

/* Private UI Context. */
typedef struct ui_ctx ui_ctx_s;

#define UI_EVENT_MASK (SDL_WINDOWEVENT | SDL_MOUSEMOTION | SDL_KEYDOWN | SDL_JOYAXISMOTION)

struct menu_ctx
{
	/* The menu to show when the user wants to go up a level. NULL if this
	 * is the root menu. */
	struct menu_ctx *parent;

	/* Name of the menu option. */
	const char *title;

	/* Help text to show for the menu option.
	 * NULL if no help text is available. */
	const char *help;

	enum
	{
		/* Small list using large icons. */
		STYLE_MENU_LARGE_LIST,

		/* Long list. */
		STYLE_MENU_LIST,

		/* Grid of options. */
		STYLE_MENU_GRID
	} style_menu;

	enum
	{
		/* The items in this menu do not change from initilisation. */
		LIST_TYPE_STATIC,

		/* The items in this menu will be created when entering the
		 * menu. */
		LIST_TYPE_DYNAMIC
	} list_type;

	union
	{
		struct
		{
			Uint32 items_nmemb;
			struct menu_item *items;
		} static_list;
		struct
		{
			/* Private pointer passed to function. May be NULL. */
			void *priv;

			/* Function to call when the dynamic menu is selected.
			 *
			 * \param priv	Private pointer.
			 * \param items	Pointer to save menu items.
			 * \return	Number of items saved. UINT32_MAX on error.
			 */
			unsigned (*fill_items)(void *priv, struct menu_item **items);
		} dynamic_list;
	} items;

	/* The currently highlighted item. Must be 0 on initialisation.
	 * 0 is the first menu item. */
	unsigned item_selected;
};

struct menu_item
{
	char *name;
	char *help;

	enum
	{
		/* Opens a sub menu. */
		MENU_SUB_MENU,

		/* Executes a function. */
		MENU_EXEC_FUNC,

		/* Sets the value of an integer. */
		MENU_SET_VAL
	} op;

	union
	{
		/* Pointer to sub menu. */
		struct menu_ctx *sub_menu;

		/* Pointer to function to execute if item selected. */
		struct
		{
			void *ctx;
			void (*func)(void *ctx);
		} exec_func;

		/* Pointer to integer to set. */
		struct
		{
			int val;
			int *set;
		} set_val;
	} param;

	SDL_Colour bg;
	SDL_Colour fg;
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
 * \param root	Pointer to root (or main) menu. Must remain valid until after
 *		ui_exit is called.
 * \return	UI context. NULL on error.
 */
ui_ctx_s *ui_init(SDL_Window *win, struct menu_ctx *root);

SDL_bool ui_should_redraw(ui_ctx_s *ctx);

void ui_exit(ui_ctx_s *ctx);
