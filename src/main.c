/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <font.h>
#include <menu.h>
#include <SDL.h>
#include <ui.h>
//#include <win_dirent.h>

static void loop(SDL_Window *win, SDL_Renderer *ren, ui_ctx *ui)
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
			case SDLK_z:
				ui_input(ui, SDL_CONTROLLER_BUTTON_A);
				break;

			case SDLK_x:
			case SDLK_BACKSPACE:
				ui_input(ui, SDL_CONTROLLER_BUTTON_B);
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
		SDL_Delay(1);

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
	ui_ctx *ui;
	font_ctx *font;

	struct menu_item root_items[] = {
		{
		"Continue", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, ui_nop_cb },
		.style = {.bg = {.r = 0x1C, .g = 0x4D, .b = 0x16, .a = SDL_ALPHA_OPAQUE},
		.selected_outline = {.r = 0x45, .g = 0xB3, .b = 0x32, .a = SDL_ALPHA_OPAQUE}}
		},
		{
		"Open", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, ui_nop_cb },
		.style = {.bg = {.r = 0x40, .g = 0x30, .b = 0x59, .a = SDL_ALPHA_OPAQUE},
		.selected_outline = {.r = 0xA2, .g = 0x80, .b = 0xFF, .a = SDL_ALPHA_OPAQUE}}
		},
		{
		"Exit", NULL, MENU_SET_VAL, .param.set_val = { 1, &quit },
		.style = {.bg = {.r = 0x59, .g = 0x00, .b = 0x00, .a = SDL_ALPHA_OPAQUE},
		.selected_outline = {.r = 0xD9, .g = 0x00, .b = 0x00, .a = SDL_ALPHA_OPAQUE}}
		}
	};
	struct menu_ctx root_menu = {
		.parent = NULL, .title = "Main Menu", .help = NULL,
		.item_selected = 0,
		.list_type = LIST_TYPE_STATIC,
		.items_u.static_list =
		{
		.items_nmemb = SDL_arraysize(root_items),
		.items = root_items
		}
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

	ren = SDL_CreateRenderer(win, -1,
		SDL_RENDERER_ACCELERATED |
		SDL_RENDERER_PRESENTVSYNC |
		SDL_RENDERER_TARGETTEXTURE);
	if(ren == NULL)
		goto err;

	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
	SDL_SetWindowMinimumSize(win, 320, 240);
	font = FontStartup(ren);
	if(font == NULL)
		goto err;

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	ui = ui_init(win, &root_menu, font);

	while(SDL_QuitRequested() == SDL_FALSE && quit == 0)
		loop(win, ren, ui);

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
