#include "SDL_log.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_COMPILE_ERROR_TEXT

#include "lodepng.h"
#include "minctest.h"
#include <stdlib.h>

#include <font.h>
#include <menu.h>
#include <SDL.h>
#include <ui.h>

#define TARGET_WIDTH 1280
#define TARGET_HEIGHT 720

static SDL_Renderer *rend = NULL;
static SDL_Surface *surf = NULL;

struct item_priv ui_styles[] = {
	{ .bg = {.r = 0x1C, .g = 0x4D, .b = 0x16, .a = SDL_ALPHA_OPAQUE},
		.fg = {.r = 0x45, .g = 0xB3, .b = 0x32, .a = SDL_ALPHA_OPAQUE }},
	{ .bg = {.r = 0x40, .g = 0x30, .b = 0x59, .a = SDL_ALPHA_OPAQUE},
		.fg = {.r = 0xA2, .g = 0x80, .b = 0xFF, .a = SDL_ALPHA_OPAQUE }},
	{ .bg = {.r = 0x59, .g = 0x00, .b = 0x00, .a = SDL_ALPHA_OPAQUE},
		.fg = {.r = 0xD9, .g = 0x00, .b = 0x00, .a = SDL_ALPHA_OPAQUE }}
};

struct ui_expected {
	const char *png_filename;
	unsigned char *pixels;
} ui_exp[] = {
	{ "test/img/main_menu_continue.png",	NULL },
	{ "test/img/main_menu_open.png",	NULL },
	{ "test/img/main_menu_quit.png",	NULL }
};

#define MAIN_MENU_CONTINUE_PNG 0
#define MAIN_MENU_OPEN_PNG 1
#define MAIN_MENU_QUIT_PNG 2

static unsigned char *png_to_pixels(const char *png_filename)
{
	unsigned int w, h, ret;
	unsigned char *expected;
	ret = lodepng_decode24_file(&expected, &w, &h, png_filename);
	SDL_assert_always(ret == 0);
	SDL_assert_always(w == TARGET_WIDTH);
	SDL_assert_always(h == TARGET_HEIGHT);
	return expected;
}

void test_main_menu_look(void)
{
	struct ui_ctx *ui;
	font_ctx *font;
	int set_val_test;
	unsigned w = TARGET_WIDTH;
	unsigned h = TARGET_HEIGHT;

	struct menu_item root_items[] = {
		{
		"Continue", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, NULL },
		.priv = &ui_styles[0]
		},
		{
		"Open", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, NULL },
		.priv = &ui_styles[1]
		},
		{
		"Exit", NULL, MENU_SET_VAL, .param.set_val = {.set = &set_val_test, .val = 1 },
		.priv = &ui_styles[2]
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

	SDL_RenderClear(rend);

	font = FontStartup(rend);
	SDL_assert_always(font != NULL);

	ui = ui_init_renderer(rend, 96.0f, SDL_PIXELFORMAT_RGB24,
		&root_menu, font);

	/* Check that UI context is initialised successfully. */
	lok(ui != NULL);

	ui_render_frame(ui);
	SDL_RenderPresent(rend);

	/* Test that the main menu UI is correctly rendered. In this test, the
	* Continue menu option should be selected. */
	{
		int res;
		res = memcmp(ui_exp[MAIN_MENU_CONTINUE_PNG].pixels, surf->pixels, (size_t)w * h * 3);
		lok(res == 0);
		if(res != 0)
			SDL_SaveBMP(surf, ui_exp[MAIN_MENU_CONTINUE_PNG].png_filename);
	}

	/* Select and check the "Open" menu option. */
	ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
	ui_render_frame(ui);
	{
		int res;
		res = memcmp(ui_exp[MAIN_MENU_OPEN_PNG].pixels, surf->pixels, (size_t)w * h * 3);
		lok(res == 0);
		if(res != 0)
			SDL_SaveBMP(surf, ui_exp[MAIN_MENU_OPEN_PNG].png_filename);
	}

	/* Select and check the "Exit" menu option. */
	ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
	ui_render_frame(ui);
	{
		int res;
		res = memcmp(ui_exp[MAIN_MENU_QUIT_PNG].pixels, surf->pixels, (size_t)w * h * 3);
		lok(res == 0);
		if(res != 0)
			SDL_SaveBMP(surf, ui_exp[MAIN_MENU_QUIT_PNG].png_filename);
	}

	{
		unsigned int w, h, ret;
		int res;
		unsigned char *expected;
		ret = lodepng_decode24_file(&expected, &w, &h,
			"test/img/main_menu_quit.png");
		SDL_assert_always(ret == 0);

		lok(memcmp(expected, surf->pixels, (size_t)w * h * 3) == 0);

		/* Check that the cursor does not loop or go "missing" if there
		* are no more items in the main menu. */
		ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		ui_render_frame(ui);

		lok(memcmp(expected, surf->pixels, (size_t)w * h * 3) == 0);
		res = memcmp(expected, surf->pixels, (size_t)w * h * 3);
		lok(res == 0);
		if(res != 0)
			SDL_SaveBMP(surf, "main_menu_quit_res.bmp");

		free(expected);
	}

	/* Check that executing the "Exit" option works. This will change the
	* value of the variable set_val_test to 1. This will not exit the
	* application. */
	ui_input(ui, SDL_CONTROLLER_BUTTON_A);
	lequal(set_val_test, 1);

	FontExit(font);
	ui_exit(ui);
	return;
}

int main(void)
{
	if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0)
		goto err;

	/* Prepare expected UI images. */
	for(unsigned i = 0; i < SDL_arraysize(ui_exp); i++)
		ui_exp[i].pixels = png_to_pixels(ui_exp[i].png_filename);

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	surf = SDL_CreateRGBSurfaceWithFormat(0, TARGET_WIDTH, TARGET_HEIGHT, 32,
		SDL_PIXELFORMAT_RGB24);
	SDL_assert_always(surf != NULL);

	rend = SDL_CreateSoftwareRenderer(surf);
	SDL_assert_always(rend != NULL);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	/* Begin tests. */
	lrun("Test Main Menu UI Look", test_main_menu_look);
	lresults();

	/* Free UI images. */
	for(unsigned i = 0; i < SDL_arraysize(ui_exp); i++)
		free(ui_exp[i].pixels);

	SDL_Quit();
	return lfails != 0;

err:
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
	return 1;
}
