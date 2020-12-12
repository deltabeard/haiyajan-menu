/**
 * Copyright (C) 2020 by Mahyar Koshkouei <mk@deltabeard.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include <SDL.h>

#define MENU_SELECT_ITEM(menu, sel)				\
	do{							\
		if((sel - 1) < menu->items_nmemb)		\
			menu->item_selected = sel;		\
	}while(0)

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

	void *priv;
};

typedef enum
{
	/* Go back to the previous item.
	 * Could be used when user presses UP. */
	MENU_INSTR_PREV_ITEM,

	/* Go to next item in menu.
	 * Could be used when user presses DOWN. */
	MENU_INSTR_NEXT_ITEM,

	/* Go to parent menu if one exists.
	 * Could be used when user presses BACKSPACE. */
	MENU_INSTR_PARENT_MENU,

	/* Execute item operation.
	 * Could be used when user presses ENTER. */
	MENU_INSTR_EXEC_ITEM
} menu_instruction_e;

struct menu_ctx *menu_instruct(struct menu_ctx *ctx, menu_instruction_e instr);
