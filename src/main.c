/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <menu.h>
#include <SDL.h>
#include <ui.h>

static void loop(SDL_Window *win, SDL_Renderer *rend, struct ui_s *ui)
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

static void ui_nop_cb(void *ctx)
{
	(void)ctx;
	return;
}

int main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_Renderer *rend = NULL;
	int ret;
	struct ui_s ui = {0};
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

	{
		ui_ctx *ui_priv;
		Uint32 texture_format = SDL_GetWindowPixelFormat(win);
		ui_priv = ui_init(rend, texture_format);
	}

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
