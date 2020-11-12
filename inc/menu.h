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
#include <stdlib.h>

#define MENU_SELECT_ITEM(menu, sel)				\
	do{							\
		if((sel - 1) < menu->items_nmemb)		\
			menu->item_selected = sel;		\
	}while(0)

typedef struct menu_ctx
{
	struct menu_ctx_s *parent;
	const char *title;
	const char *help;
	enum
	{
		STYLE_MAIN_MENU,
		STYLE_LIST_MENU,
	} style;

	unsigned long item_selected;
	enum
	{
		LIST_TYPE_STATIC,
		//LIST_TYPE_DYNAMIC
	} list_type;
	union
	{
		struct
		{
			unsigned long items_nmemb;
			struct menu_item *items;
		} static_list;
	} items_u;
} menu_ctx_s;

typedef enum
{
	/* Go back to the previous item.
	 * Could be used when user presses UP.
	*/
	MENU_INSTR_PREV_ITEM,

	/* Go to next item in menu.
	 * Could be used when user presses DOWN.
	*/
	MENU_INSTR_NEXT_ITEM,

	/* Go to parent menu if one exists.
	 * Could be used when user presses BACKSPACE.
	*/
	MENU_INSTR_PARENT_MENU,

	/* Execute item operation.
	 * Could be used when user presses ENTER.
	*/
	MENU_INSTR_EXEC_ITEM
} menu_instruction_e;

struct menu_item
{
	char *name;
	char *help;
	enum menu_op_e
	{
		/* Opens a sub menu. */
		MENU_SUB_MENU,

		/* Executes a function. */
		MENU_EXEC_FUNC,

		/* Sets the value of an integer. */
		MENU_SET_VAL
	} op;

	union param_u
	{
		/* Pointer to sub menu. */
		struct menu_ctx *sub_menu;

		/* Pointer to function to execute if item selected. */
		struct exec_func_s
		{
			void *ctx;
			void (*func)(void *ctx);
		} exec_func;

		/* Pointer to integer to set. */
		struct set_val_s
		{
			int val;
			int *set;
		} set_val;
	} param;

	void *priv;

	SDL_Colour bg;
	SDL_Colour selected_outline;
};

/**
 * Add an item to a given menu. First item shown first in menu.
 */
void menu_set_items(struct menu_ctx *menu, unsigned long nmemb, struct menu_item *items);
struct menu_ctx *menu_instruct(struct menu_ctx *ctx, menu_instruction_e instr);
