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
			//ui_process_event(ui, &e);
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

void onclick_function_debug(ui_e *element)
{
	SDL_Log("Element %p clicked", element);
}

/**
 * Process input arguments and initialise libraries.
*/
int main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_Renderer *ren = NULL;
	int ret;
	static Sint32 quit = SDL_FALSE;
	ui_ctx_s *ui;

	ui_e ui_elements[] = {
		{
			.type = UI_ELEM_TYPE_TILE,
			.tile = {
				.label = "Continue",
				.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
				.tile_shape = TILE_SHAPE_SQUARE,
				.tile_size = TILE_SIZE_LARGE,
				.icon = 0xE768,
				.help = NULL,
				/* Persian Green */
				.bg = { .r = 0x00, .g = 0xA3, .b = 0x98, .a = SDL_ALPHA_OPAQUE },
				.fg = { 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE },
				.disabled = SDL_FALSE,
				.onclick = ONCLICK_EXECUTE_FUNCTION,
				.onclick_event = { .execute_function = onclick_function_debug },
				.user = NULL
			}
		},
		{
			.type = UI_ELEM_TYPE_TILE,
			.tile = {
				.label = "経営に関する情報",
				.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
				.tile_shape = TILE_SHAPE_SQUARE,
				.tile_size = TILE_SIZE_LARGE,
				.icon = 0xE8B7,
				.help = NULL,
				/* Pale Spring Bud */
				.bg = {.r = 0xEB, .g = 0xF8, .b = 0xB8, .a = SDL_ALPHA_OPAQUE},
				.fg = { 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE },
				.disabled = SDL_FALSE,
				.onclick = ONCLICK_EXECUTE_FUNCTION,
				.onclick_event = {.execute_function = onclick_function_debug },
				.user = NULL
			}
		},
		{
			.type = UI_ELEM_TYPE_TILE,
			.tile = {
				.label = "بنی آدم",
				.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
				.tile_shape = TILE_SHAPE_SQUARE,
				.tile_size = TILE_SIZE_LARGE,
				.icon = 0xE7E8,
				.help = NULL,
				/* Auburn */
				.bg = {.r = 0x9E, .g = 0x2A, .b = 0x2B, .a = SDL_ALPHA_OPAQUE},
				.fg = { 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE },
				.disabled = SDL_FALSE,
				.onclick = ONCLICK_SET_SIGNED_VARIABLE,
				.onclick_event = {.signed_variable = { &quit, SDL_TRUE }},
				.user = NULL
			}
		},
		{
			.type = UI_ELEM_TYPE_END,
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
	ui = ui_init(win, ui_elements);
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
