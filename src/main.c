/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020-2022 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#ifdef __EMSCRIPTEN__
# include <emscripten.h>
#endif

#include "signal.h"
#include "SDL.h"
#include "ui.h"

void onclick_function_debug(const struct ui_element *element);

static Uint32 quit = 0;

static unsigned ticks_element_num(void *user_ctx)
{
	(void) user_ctx;
	return 1;
}

static int ticks_elemen_get(unsigned memb,
	struct ui_element *element, char *label, unsigned label_sz,
	void *user_ctx)
{
	(void) user_ctx;

	/* There is only one member in this dynamic entry. */
	if(memb != 0)
		return 0;

	SDL_snprintf(label, label_sz, "Ticks: %" SDL_PRIu64, SDL_GetTicks64());

	element->type = UI_ELEM_TYPE_LABEL;
	element->label = label;
	element->elem.label.style = FONT_STYLE_REGULAR;

	return 1;
}

static unsigned power_element_num(void *user_ctx)
{
	(void) user_ctx;
	return 1;
}

static int power_elemen_get(unsigned memb,
	struct ui_element *element, char *label, unsigned label_sz,
		void *user_ctx)
{
	int secs, pct;
	SDL_PowerState state;

	(void) user_ctx;

	/* There is only one member in this dynamic entry. */
	if(memb != 0)
		return 0;

	state = SDL_GetPowerInfo(&secs, &pct);

	switch(state)
	{
	case SDL_POWERSTATE_ON_BATTERY:
		SDL_snprintf(label, label_sz, "Running on battery with %d%% "
					      "remaining", pct);
		break;
	case SDL_POWERSTATE_NO_BATTERY:
		SDL_strlcpy(label, "Running on external power", label_sz);
		break;
	case SDL_POWERSTATE_CHARGING:
		SDL_snprintf(label, label_sz, "Charging battery at %d%%", pct);
		break;
	case SDL_POWERSTATE_CHARGED:
		SDL_strlcpy(label, "Battery fully charged", label_sz);
		break;
	case SDL_POWERSTATE_UNKNOWN:
	default:
		SDL_strlcpy(label, "Unknown power state", label_sz);
		break;

	}

	element->type = UI_ELEM_TYPE_LABEL;
	element->label = label;
	element->elem.label.style = FONT_STYLE_REGULAR;

	return 1;
}

extern const struct ui_element ui_elements[];

static const struct ui_element sub_menu_1[] = {
	{
		.type = UI_ELEM_TYPE_TILE,
		.label = "Label Outside Top",
		.elem.tile = {
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_TOP,
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
		.type = UI_ELEM_TYPE_DYNAMIC,
		.label = "Battery Status",
		.elem.dynamic = {
			.number_of_elements = power_element_num,
			.get_element = power_elemen_get
		}
	},
	{
		.type = UI_ELEM_TYPE_DYNAMIC,
		.label = "Ticks",
		.elem.dynamic = {
			.number_of_elements = ticks_element_num,
			.get_element = ticks_elemen_get
		}
	},
	{
		.type = UI_ELEM_TYPE_TILE,
		.label = "Back",
		.elem.tile = {
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
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
		.label = "Main Menu",
		.elem.label = {
			.style = FONT_STYLE_HEADER
		}
	},
	{
		.type = UI_ELEM_TYPE_TILE,
		.label = "Label",
		.elem.tile = {
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
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
		.label = "Label Outside Middle",
		.elem.tile = {
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_MIDDLE,
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
#ifndef __EMSCRIPTEN__
	{
		.type = UI_ELEM_TYPE_TILE,
		.label = "Exit",
		.elem.tile = {
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
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
#endif
	{
		.type = UI_ELEM_TYPE_TILE,
		.label = "Go to sub-menu",
		.elem.tile = {
			.label_placement = LABEL_PLACEMENT_OUTSIDE_RIGHT_BOTTOM,
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

struct loop_ctx
{
	SDL_Renderer *ren;
	ui_ctx_s *ui;
};

static void loop(void *userdata)
{
	struct loop_ctx *ctx = userdata;
	SDL_Renderer *ren = ctx->ren;
	ui_ctx_s *ui = ctx->ui;
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

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>

# if (_WIN32_WINNT >= 0x0603)
#  include <shellscalingapi.h>
# endif

void set_dpi_awareness(void)
{
# if (_WIN32_WINNT >= 0x0605)
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
# elif (_WIN32_WINNT >= 0x0603)
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
# elif (_WIN32_WINNT >= 0x0600)
	SetProcessDPIAware();
# elif defined(__MINGW64__)
	SetProcessDPIAware();
# endif
	return;
}
#endif

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

#ifdef _WIN32
	set_dpi_awareness();
#endif

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

	struct loop_ctx loop_ctx = { .ren = ren, .ui = ui };
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(loop, &loop_ctx, 0, 1);
#else
	while(SDL_QuitRequested() == SDL_FALSE && quit == 0)
		loop(&loop_ctx);
#endif

	ui_exit(ui);

out:
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();

	return ret;

err:
	{
		char buf[512];
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
