/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 */

#include <SDL.h>
#include <ui.h>
//#include <win_dirent.h>

static void loop(SDL_Renderer *ren, ui_ctx_s *ui)
{
	SDL_Event e;

	while(SDL_PollEvent(&e))
	{
		if(e.type & UI_EVENT_MASK)
		{
			ui_process_event(ui, &e);
		}
	}

	if(ui_should_redraw(ui) == SDL_TRUE)
	{
		ui_render_frame(ui);
		SDL_RenderPresent(ren);
	}
	else
	{
		const int frame_delay = 5;
		SDL_Delay(frame_delay);
	}


	return;
}

static void ui_nop_cb(void *ctx)
{
	(void)ctx;
	return;
}

/**
 * Process input arguments and initialise libraries.
*/
int main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_Renderer *ren = NULL;
	int ret;
	static int quit = SDL_FALSE;
	ui_ctx_s *ui;

	struct menu_item root_items[] = {
		{
		"Continue", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, ui_nop_cb },
		.bg = {.r = 0x1C, .g = 0x4D, .b = 0x16, .a = SDL_ALPHA_OPAQUE},
		.fg = {.r = 0x45, .g = 0xB3, .b = 0x32, .a = SDL_ALPHA_OPAQUE}
		},
		{
		"Open", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, ui_nop_cb },
		.bg = {.r = 0x40, .g = 0x30, .b = 0x59, .a = SDL_ALPHA_OPAQUE},
		.fg = {.r = 0xA2, .g = 0x80, .b = 0xFF, .a = SDL_ALPHA_OPAQUE}
		},
		{
		"Exit", NULL, MENU_SET_VAL, .param.set_val = { 1, &quit },
		.bg = {.r = 0x59, .g = 0x00, .b = 0x00, .a = SDL_ALPHA_OPAQUE},
		.fg = {.r = 0xD9, .g = 0x00, .b = 0x00, .a = SDL_ALPHA_OPAQUE}
		}
	};
	struct menu_ctx root_menu = {
		.parent = NULL, .title = "Main Menu", .help = NULL,
		.item_selected = 0,
		.list_type = LIST_TYPE_STATIC,
		.items.static_list = {
			.items_nmemb = SDL_arraysize(root_items),
			.items = root_items
		}
	};

	/* Input arguments are currently unused. */
	(void)argc;
	(void)argv;

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER |
			SDL_INIT_EVENTS | SDL_INIT_TIMER);
	if(ret != 0)
		goto err;

	win = SDL_CreateWindow("Haiyajan UI",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
		SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED);
	if(win == NULL)
		goto err;

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED |
		SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if(ren == NULL)
		goto err;


	/* TODO: Allow even smaller screens. */
	SDL_SetWindowMinimumSize(win, 320, 240);
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

	/* Initialise user interface context. */
	ui = ui_init(win, &root_menu);
	if(ui == NULL)
		goto err;

	while(SDL_QuitRequested() == SDL_FALSE && quit == 0)
		loop(ren, ui);

	ui_exit(ui);

out:
	SDL_DestroyRenderer(ren);
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
