/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020-2022 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 */

#include <signal.h>
#include <SDL.h>
#include <ui.h>

void onclick_function_debug(const struct ui_element *element);

static Uint32 quit = 0;

extern const struct ui_element ui_elements[];

static const struct ui_element sub_menu_1[] = {
	{
		.type = UI_ELEM_TYPE_TILE,
		.elem.tile = {
			.label = "First sub-menu tile",
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
			.tile_size = TILE_SIZE_LARGE,
			.icon = 0xE8B7,
			.help = NULL,
			.bg = { 0, 0, 0, SDL_ALPHA_OPAQUE},
			.fg = { 0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE},
			.disabled = SDL_FALSE,
			.onclick = {
				.action = UI_EVENT_EXECUTE_FUNCTION,
				.action_data.execute_function = {
					onclick_function_debug
				},
			},
			.user = NULL
		}
	},
	{
		.type = UI_ELEM_TYPE_TILE,
		.elem.tile = {
			.label = "Back",
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
			.tile_size = TILE_SIZE_LARGE,
			.icon = 0xE8B7,
			.help = NULL,
			.bg = { 0, 0, 0, SDL_ALPHA_OPAQUE},
			.fg = { 0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE},
			.disabled = SDL_FALSE,
			.onclick = {
				.action = UI_EVENT_GOTO_ELEMENT,
				.action_data.goto_element = {
					ui_elements
				},
			},
			.user = NULL
		}
	},
	{
		.type = UI_ELEM_TYPE_END
	}
};

const struct ui_element ui_elements[] = {
	{
		.type = UI_ELEM_TYPE_LABEL,
		.elem.label = {
			.label = "Main Menu",
			.style = LABEL_STYLE_HEADER
		}
	},
	{
		.type = UI_ELEM_TYPE_TILE,
		.elem.tile = {
			.label = "Label",
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
			.tile_size = TILE_SIZE_LARGE,
			.icon = 0xE768,
			.help = NULL,
			/* Persian Green */
			.bg = {.r = 0x00, .g = 0xA3, .b = 0x98, .a = SDL_ALPHA_OPAQUE},
			.fg = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = SDL_ALPHA_OPAQUE},
			.disabled = SDL_FALSE,
			.onclick = {
				.action = UI_EVENT_EXECUTE_FUNCTION,
				.action_data.execute_function = {
					onclick_function_debug
				},
			},
			.user = NULL
		}
	},
	{
		.type = UI_ELEM_TYPE_TILE,
		.elem.tile = {
			.label = "Second Label",
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
			.tile_size = TILE_SIZE_LARGE,
			.icon = 0xE8B7,
			.help = NULL,
			/* Persian Blue */
			.bg = {.r = 0x1C, .g = 0x39, .b = 0xBB, .a = SDL_ALPHA_OPAQUE},
			.fg = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = SDL_ALPHA_OPAQUE},
			.disabled = SDL_FALSE,
			.onclick = {
				.action = UI_EVENT_EXECUTE_FUNCTION,
				.action_data.execute_function = {
					onclick_function_debug
				},
			},
			.user = NULL
		}
	},
	{
		.type = UI_ELEM_TYPE_TILE,
		.elem.tile = {
			.label = "Exit",
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
			.tile_size = TILE_SIZE_LARGE,
			.icon = 0xE7E8,
			.help = NULL,
			/* Auburn */
			.bg = {.r = 0x9E, .g = 0x2A, .b = 0x2B, .a = SDL_ALPHA_OPAQUE},
			.fg = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = SDL_ALPHA_OPAQUE},
			.disabled = SDL_FALSE,
			.onclick = {
				.action = UI_EVENT_SET_UNSIGNED_VARIABLE,
				.action_data.unsigned_variable = {
					.variable = &quit,
					.val = 1
				},
			},
			.user = NULL
		}
	},
	{
		.type = UI_ELEM_TYPE_TILE,
		.elem.tile = {
			.label = "Go to sub-menu",
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
			.tile_size = TILE_SIZE_LARGE,
			.icon = 0xE8B7,
			.help = NULL,
			/* Persian Blue */
			.bg = {.r = 0x1C, .g = 0x39, .b = 0xBB, .a = SDL_ALPHA_OPAQUE},
			.fg = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = SDL_ALPHA_OPAQUE},
			.disabled = SDL_FALSE,
			.onclick = {
				.action = UI_EVENT_GOTO_ELEMENT,
				.action_data.goto_element = {
					sub_menu_1
				},
			},
			.user = NULL
		}
	},
	{
		.type = UI_ELEM_TYPE_END,
	}
};

static void loop(SDL_Renderer *ren, ui_ctx_s *ui)
{
	SDL_Event e;
	SDL_Texture *ui_tex;

	while(SDL_PollEvent(&e))
	{
		if(e.type & UI_EVENT_MASK)
		{
			ui_process_event(ui, &e);
		}
	}

	ui_tex = ui_render_frame(ui);
	SDL_SetRenderTarget(ren, NULL);
	/* The UI texture is copied to the entire screen, so a RenderClear is
	 * not required. */
	//SDL_RenderClear(ren);
	SDL_RenderCopy(ren, ui_tex, NULL, NULL);
	SDL_RenderPresent(ren);

	return;
}

void onclick_function_debug(const struct ui_element *element)
{
	SDL_Log("Element %p clicked", (void *)element);
}

void print_fps(void)
{
	static unsigned frames = 0;
	static unsigned tim = 0;
	unsigned now;

	if(tim == 0)
	{
		tim = SDL_GetTicks();
		return;
	}

	frames++;
	now = SDL_GetTicks();

	/* Print FPS every second. */
	if(now - tim > 1000)
	{
		float fps;
		fps = ((float)frames/(float)(now - tim)) * 1000.0f;
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
				"FPS: %.2f", fps);
		frames = 0;
		tim = now;
	}

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
	ui_ctx_s *ui;

	/* Input arguments are currently unused. */
	(void)argc;
	(void)argv;

	SDL_SetMainReady();
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	ret = SDL_Init(
		SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER |
		SDL_INIT_EVENTS | SDL_INIT_TIMER);
	if(ret != 0)
		goto err;

	win = SDL_CreateWindow("Haiyajan UI",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		UI_DEFAULT_WINDOW_WIDTH, UI_DEFAULT_WINDOW_HEIGHT,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
		SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED);
	if(win == NULL)
		goto err;

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED |
					  SDL_RENDERER_PRESENTVSYNC |
					  SDL_RENDERER_TARGETTEXTURE);
	if(ren == NULL)
		goto err;

	/* TODO: Allow even smaller screens. */
	SDL_SetWindowMinimumSize(win, UI_MIN_WINDOW_WIDTH,
		UI_MIN_WINDOW_HEIGHT);
	SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

	/* Initialise user interface context. */
	ui = ui_init(win, ui_elements);
	if(ui == NULL)
		goto err;

	while(SDL_QuitRequested() == SDL_FALSE && quit == 0)
	{
		loop(ren, ui);
		print_fps();
	}

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
			"A critical error has occurred, and Haiyajan must "
			"now close.\n"
			"Error %d: %s\n", ret, SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Critical Error", buf, win);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			"Error %d: %s", ret, SDL_GetError());
	}

	goto out;
}
