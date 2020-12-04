#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_COMPILE_ERROR_TEXT

#include "lodepng.h"
#include "minctest.h"

#include <font.h>
#include <menu.h>
#include <SDL.h>
#include <ui.h>

#define TARGET_WIDTH 1280
#define TARGET_HEIGHT 720

static SDL_Renderer *rend = NULL;
static SDL_Surface *surf = NULL;

void test_main_menu_look(void)
{
	ui_ctx *ui;
	font_ctx *font;
	int set_val_test;

	struct menu_item root_items[] = {
		{
		"Continue", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, NULL },
		.style = {.bg = {.r = 0x1C, .g = 0x4D, .b = 0x16, .a = SDL_ALPHA_OPAQUE},
		.selected_outline = {.r = 0x45, .g = 0xB3, .b = 0x32, .a = SDL_ALPHA_OPAQUE}}
		},
		{
		"Open", NULL, MENU_EXEC_FUNC, .param.exec_func = { NULL, NULL },
		.style = {.bg = {.r = 0x40, .g = 0x30, .b = 0x59, .a = SDL_ALPHA_OPAQUE},
		.selected_outline = {.r = 0xA2, .g = 0x80, .b = 0xFF, .a = SDL_ALPHA_OPAQUE}}
		},
		{
		"Exit", NULL, MENU_SET_VAL, .param.set_val = {.set = &set_val_test, .val = 1 },
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
		int w, h, ret;
		unsigned char *expected;
		ret = lodepng_decode24_file(&expected, &w, &h,
			"test/img/main_menu_continue.png");
		SDL_assert_always(ret == 0);

		lok(memcmp(expected, surf->pixels, (size_t)w * h * 3) == 0);
		free(expected);
	}

	/* Select and check the "Open" menu option. */
	ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
	ui_render_frame(ui);
	{
		int w, h, ret;
		unsigned char *expected;
		ret = lodepng_decode24_file(&expected, &w, &h,
			"test/img/main_menu_open.png");
		SDL_assert_always(ret == 0);

		lok(memcmp(expected, surf->pixels, (size_t)w * h * 3) == 0);
		free(expected);
	}

	/* Select and check the "Exit" menu option. */
	ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
	ui_render_frame(ui);
	{
		int w, h, ret;
		unsigned char *expected;
		ret = lodepng_decode24_file(&expected, &w, &h,
			"test/img/main_menu_quit.png");
		SDL_assert_always(ret == 0);

		lok(memcmp(expected, surf->pixels, (size_t)w * h * 3) == 0);
		free(expected);

		/* Check that the cursor does not loop or go "missing" if there
		* are no more items in the main menu. */
		ui_input(ui, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		ui_render_frame(ui);
		ret = lodepng_decode24_file(&expected, &w, &h,
			"test/img/main_menu_quit.png");
		SDL_assert_always(ret == 0);

		lok(memcmp(expected, surf->pixels, (size_t)w * h * 3) == 0);
		free(expected);
	}

	/* Check that executing the "Exit" option works. This will change the
	* value of the variable set_val_test to 1. This will not exit the
	* application. */
	ui_input(ui, SDL_CONTROLLER_BUTTON_A);
	lequal(set_val_test, 1);

	ui_exit(ui);
	return;
}

int main(void)
{
	if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0)
		goto err;

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	surf = SDL_CreateRGBSurfaceWithFormat(0, TARGET_WIDTH, TARGET_HEIGHT, 32,
		SDL_PIXELFORMAT_RGB24);
	SDL_assert_always(surf != NULL);

	rend = SDL_CreateSoftwareRenderer(surf);
	SDL_assert_always(rend != NULL);

	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

	lrun("Test Main Menu UI Look", test_main_menu_look);
	lresults();

	SDL_Quit();
	return lfails != 0;

err:
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
	return 1;
}