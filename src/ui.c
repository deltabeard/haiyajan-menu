/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <menu.h>
#include <SDL.h>
#include <ui.h>

int ui_redraw(struct ui_s *ui, SDL_Renderer *rend)
{
	SDL_Rect main_menu_box = { 150, 50, 100, 100 };
	const unsigned box_spacing = 120;
	const SDL_Colour main_menu_col[3] = {
		{ 0x1C, 0x4D, 0x16, SDL_ALPHA_OPAQUE },
		{ 0x40, 0x30, 0x59, SDL_ALPHA_OPAQUE },
		{ 0x59, 0x00, 0x00, SDL_ALPHA_OPAQUE }
	};
	const SDL_Colour main_menu_sel_col[3] = {
		{ 0x45, 0xB3, 0x32, SDL_ALPHA_OPAQUE },
		{ 0xA2, 0x80, 0xFF, SDL_ALPHA_OPAQUE },
		{ 0xD9, 0x00, 0x00, SDL_ALPHA_OPAQUE }
	};

	for(unsigned item = 0; item < ui->current->items_nmemb; item++)
	{
		const SDL_Colour fg = main_menu_col[item];
		const SDL_Colour bg = main_menu_sel_col[item];

		if(item == ui->current->item_selected)
		{
			SDL_Rect bg_box = main_menu_box;
			bg_box.x -= 5;
			bg_box.y -= 5;
			bg_box.w += 10;
			bg_box.h += 10;
			SDL_SetRenderDrawColor(rend, bg.r, bg.g, bg.b, bg.a);
			SDL_RenderFillRect(rend, &bg_box);
		}

		SDL_SetRenderDrawColor(rend, fg.r, fg.g, fg.b, fg.a);
		SDL_RenderFillRect(rend, &main_menu_box);
		main_menu_box.y += box_spacing;
	}

	return 0;
}

void ui_input(struct ui_s *ui, SDL_GameControllerButton btn)
{
	switch(btn)
	{
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		ui->current = menu_instruct(ui->current, MENU_INSTR_PREV_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		ui->current = menu_instruct(ui->current, MENU_INSTR_NEXT_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_A:
		ui->current = menu_instruct(ui->current, MENU_INSTR_EXEC_ITEM);
		break;

	case SDL_CONTROLLER_BUTTON_B:
		ui->current = menu_instruct(ui->current, MENU_INSTR_PARENT_MENU);
		break;

	default:
		return;
	}

	ui->redraw_required = SDL_TRUE;

	return;
}
