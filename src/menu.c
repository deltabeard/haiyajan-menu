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

#include <menu.h>
#include <stdlib.h>

void menu_set_items(struct menu_ctx* menu, unsigned long nmemb,
	struct menu_item* items)
{
	menu->items_u.static_list.items_nmemb = nmemb;
	menu->items_u.static_list.items = items;
}

struct menu_ctx* menu_instruct(struct menu_ctx* ctx, menu_instruction_e instr)
{
	struct menu_ctx* ret = ctx;

	switch (instr)
	{
	case MENU_INSTR_PREV_ITEM:
		if (ctx->item_selected > 0)
			ctx->item_selected--;

		break;

	case MENU_INSTR_NEXT_ITEM:
		if (ctx->item_selected < (ctx->items_u.static_list.items_nmemb - 1))
			ctx->item_selected++;

		break;

	case MENU_INSTR_PARENT_MENU:
		if (ctx->parent != NULL)
			ret = ctx->parent;

		break;

	case MENU_INSTR_EXEC_ITEM:
	{
		const struct menu_item* item =
			ctx->items_u.static_list.items + ctx->item_selected;

		switch (item->op)
		{
		case MENU_SUB_MENU:
			ret = item->param.sub_menu;
			break;

		case MENU_EXEC_FUNC:
		{
			void* p = item->param.exec_func.ctx;
			item->param.exec_func.func(p);
			break;
		}

		case MENU_SET_VAL:
		{
			int val = item->param.set_val.val;
			*item->param.set_val.set = val;
			break;
		}
		}
		break;
	}
	}

	return ret;
}
