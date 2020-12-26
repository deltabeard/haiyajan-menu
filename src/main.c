/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 */

#include <font.h>
#include <SDL.h>
#include <ui.h>
//#include <win_dirent.h>

#ifdef __SWITCH__
#include <switch.h>
#endif

#if defined(__ANDROID__) | defined(__TVOS__) | defined(__IPHONEOS__) | defined(__NACL__) | defined(__PNACL__) | defined(__DREAMCAST__) | defined(__PSP__)
# define APP_ALWAYS_FULLSCREEN 1
#else
# define APP_ALWAYS_FULLSCREEN 0
#endif

static void loop(SDL_Renderer *ren, ui_ctx_s *ui)
{
	SDL_Event e;

	while(SDL_PollEvent(&e))
	{
		if(e.type == SDL_KEYDOWN)
		{
			switch(e.key.keysym.sym)
			{
			case SDLK_w:
			case SDLK_UP:
				ui_input(ui, MENU_INSTR_PREV_ITEM);
				break;

			case SDLK_s:
			case SDLK_DOWN:
				ui_input(ui, MENU_INSTR_NEXT_ITEM);
				break;

			case SDLK_a:
			case SDLK_LEFT:
				/* TODO: Add skip items forward and back. */
				//ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
				break;

			case SDLK_d:
			case SDLK_RIGHT:
				//ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
				break;

			case SDLK_SPACE:
			case SDLK_RETURN:
			case SDLK_z:
				ui_input(ui, MENU_INSTR_EXEC_ITEM);
				break;

			case SDLK_x:
			case SDLK_BACKSPACE:
				ui_input(ui, MENU_INSTR_PARENT_MENU);
				break;
			}
		}
		else if(e.type & UI_EVENT_MASK)
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

int main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_Renderer *ren = NULL;
	int ret;
	static int quit = SDL_FALSE;
	ui_ctx_s *ui;
	font_ctx *font;

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

	(void)argc;
	(void)argv;

#ifdef __SWITCH__
	/* Redirect stdio to NXlink. */
	socketInitializeDefault();
	nxlinkStdio();
#endif

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER |
			SDL_INIT_EVENTS | SDL_INIT_TIMER);
	if(ret != 0)
		goto err;

#ifdef __SWITCH__
	win = SDL_CreateWindow("Haiyajan UI",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080,
		SDL_WINDOW_OPENGL);
#else
	win = SDL_CreateWindow("Haiyajan UI",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN |
		((APP_ALWAYS_FULLSCREEN != 0) ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_MAXIMIZED));
#endif
	if(win == NULL)
		goto err;

	if(APP_ALWAYS_FULLSCREEN != 0)
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
			"Starting in fullscreen mode");
	}
	else
	{
		SDL_LogVerbose(SDL_LOG_CATEGORY_VIDEO,
			"Starting in desktop mode");
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED |
		SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if(ren == NULL)
		goto err;

	SDL_SetWindowMinimumSize(win, 320, 240);
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
	font = FontStartup(ren);
	if(font == NULL)
		goto err;

	ui = ui_init(win, &root_menu, font);
	if(ui == NULL)
		goto err;

	while(SDL_QuitRequested() == SDL_FALSE && quit == 0)
		loop(ren, ui);

	ui_exit(ui);

out:
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();

#ifdef __SWITCH__
	socketExit();
#endif

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
