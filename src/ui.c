/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <menu.h>
#include <SDL.h>

struct ui_s {
	menu_ctx root;
	menu_ctx *current;
	SDL_bool redraw_required;
	int cont;
};

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
		//if(ui->root.style == MENU_STYLE_MAIN)
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

void loop(SDL_Window *win, SDL_Renderer *rend, struct ui_s *ui)
{
	SDL_Event e;
	SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_OPAQUE);

	while(SDL_PollEvent(&e))
	{
		switch(e.type)
		{
		case SDL_KEYDOWN:
			switch(e.key.keysym.sym)
			{
			case SDLK_w:
			case SDLK_UP:
				ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_UP);
				break;

			case SDLK_s:
			case SDLK_DOWN:
				ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
				break;

			case SDLK_a:
			case SDLK_LEFT:
				ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
				break;

			case SDLK_d:
			case SDLK_RIGHT:
				ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
				break;

			case SDLK_SPACE:
			case SDLK_RETURN:
				ui_input(ui, SDL_CONTROLLER_BUTTON_A);
				break;

			case SDLK_BACKSPACE:
				ui_input(ui, SDL_CONTROLLER_BUTTON_B);
				break;
			}
			break;
		}
	}

	if(ui->redraw_required == SDL_TRUE)
	{
		char title[64];

		ui->redraw_required = SDL_FALSE;
		SDL_RenderClear(rend);
		ui_redraw(ui, rend);
		
		SDL_snprintf(title, sizeof(title), "%s",
			ui->current->items[ui->current->item_selected].name);
		SDL_SetWindowTitle(win, title);
	}

	SDL_RenderPresent(rend);

	return;
}

void ui_nop_cb(void *ctx)
{
	(void)ctx;
	return;
}

void ui_quit_cb(void *ctx)
{
	SDL_Event e;
	(void) ctx;

	e.quit.type = SDL_QUIT;
	e.quit.timestamp = SDL_GetTicks();
	SDL_PushEvent(&e);
	SDL_PumpEvents();
	return;
}

int main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_Renderer *rend = NULL;
	int ret;
	struct ui_s ui = { 0 };
	static SDL_bool quit = SDL_FALSE;
	menu_ctx open_menu;
	menu_item open_items[] = {
		{ "File 1", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, ui_nop_cb }}
	};
	menu_item root_items[] = {
		{ "Continue", NULL, MENU_SET_VAL, .param.set_val = { SDL_TRUE, &ui.cont }},
		{ "Open", NULL, MENU_SUB_MENU, .param.sub_menu = &open_menu },
		{ "Exit", NULL, MENU_SET_VAL, .param.set_val = { SDL_TRUE, &quit }}
	};

	(void)argc;
	(void)argv;

	ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if(ret != 0)
		goto err;

	win = SDL_CreateWindow("Haiyajan UI",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if(win == NULL)
		goto err;

	rend = SDL_CreateRenderer(win, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if(rend == NULL)
		goto err;

	SDL_SetWindowMinimumSize(win, 320, 240);

	menu_init(&ui.root, NULL, NULL, NULL,
		SDL_arraysize(root_items), root_items);
	menu_init(&open_menu, &ui.root, "Select a file to play in Haiyajan", NULL,
		SDL_arraysize(open_items), open_items);
	ui.current = &ui.root;
	//ui.root.style = MENU_STYLE_MAIN;
	ui.redraw_required = SDL_TRUE;
	//open_menu.style = MENU_STYLE_MAIN;

	while(SDL_QuitRequested() == SDL_FALSE && quit == SDL_FALSE)
		loop(win, rend, &ui);

out:
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return ret;

err:
	{
		char buf[128];
		SDL_snprintf(buf, sizeof(buf),
			"A critical error has occured, and Haiyajan must now close.\n"
			"Error %d: %s\n", ret, SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Critical Error", buf, win);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"Error %d: %s", ret, SDL_GetError());
	}

	goto out;
}
