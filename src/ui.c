/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <menu.h>
#include <SDL.h>
#include <ui.h>

struct transition_s
{
	/* Required. */
	SDL_Texture *start;

	/* Not required. If NULL, the texture is not drawn if the starting
	* texture has flipped. Otherwise, the end texture is shown on flip. */
	SDL_Texture *end;

	/* The original coordinates and dimensions of the starting texture. */
	rel_rect begin;
	SDL_Rect begin_actual;

	/* The destination coordinates. */
	rel_rect dest;
	SDL_Rect dest_actual;

	/* The current progress of the transition. Set to zero. */
	Uint32 anim_progress_ms;

	/* The maximum duration of the transition is 800ms if the incremented
	*  transition value below is set to 1. Setting to 2 halves the
	*  duration. */
	Uint8 anim_speed;

	/* Apply uniform fading during animation. */
	unsigned fading : 1;
};

struct fixed_s
{
	SDL_Texture *tex;
	SDL_Rect coordinates;
};

struct elements_s
{
	enum type_e
	{
		ELEM_TYPE_FIXED,
		ELEM_TYPE_TRANSITION
	} type;

	union elem_u
	{
		struct fixed_s *fixed;
		struct transition_s *transition;
	} elem;

	struct elements_s *next;
};

struct ui_ctx_s
{
	SDL_Renderer *ren;
	SDL_Texture *tex;
	//void *font_atlas;
	float dpi;
};

/**
 * Ease out quint transition look up table. When transitioning from A to B,
 * the animation is defined as (B - A) * duration_ms.
*/
static double *ease_out_quint_transition = NULL;
/* The animation is for 800ms. */
static const unsigned ease_out_quint_duration_ms = 800;

#if 0
int ui_redraw(struct ui_s *ui, SDL_Renderer *rend)
{
	SDL_Rect main_menu_box = {150, 50, 100, 100};
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

	return 0;
}
#endif

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

SDL_Texture *ui_render_frame(ui_ctx *c)
{
	return 0;
}

void ui_process_event(ui_ctx *c, SDL_Event *e)
{
	/* Recalculate begin_actual coordinates on resolution and DPI change. */
	return;
}

static int init_transition_lut(void *ptr)
{
	double *lut = SDL_malloc(ease_out_quint_duration_ms * sizeof(ease_out_quint_transition));
	(void)ptr;

	/* If allocation fails, then the LUT will remain NULL. The UI driver is
	 * expected to continue working without animations in this case. */
	if(lut == NULL)
		return -1;

	for(unsigned ms = 0; ms < ease_out_quint_duration_ms; ms++)
	{
		lut[ms] = 1.0 - SDL_pow(1.0 - ((double)ms / (double)ease_out_quint_duration_ms), 5.0);
	}

	ease_out_quint_transition = lut;
	return 0;
}

ui_ctx *ui_init(SDL_Window *win, SDL_Renderer *ren)
{
	int w, h;
	ui_ctx *c;
	Uint32 texture_format;
	int display_id;

	SDL_assert_paranoid(win != NULL);
	SDL_assert_paranoid(ren != NULL);

	c = SDL_calloc(1, sizeof(ui_ctx));
	if(c == NULL)
		goto err;

	if(SDL_GetRendererOutputSize(ren, &w, &h) != 0)
		goto err;

	display_id = SDL_GetWindowDisplayIndex(win);
	if(display_id < 0)
		goto err;

	if(SDL_GetDisplayDPI(display_id, &c->dpi, NULL, NULL) != 0)
		goto err;

	/* We keep the Renderer so that we can create textures in the
	future. */
	c->ren = ren;

	texture_format = SDL_GetWindowPixelFormat(win);
	c->tex = SDL_CreateTexture(ren, texture_format,
				   SDL_TEXTUREACCESS_TARGET, w, h);
	if(c->tex == NULL)
		goto err;

	SDL_CreateThread(init_transition_lut, "UI Make LUT", NULL);

out:
	return c;

err:
	SDL_free(c);
	c = NULL;
	goto out;
}

void ui_exit(ui_ctx *c)
{
	SDL_free(c);
	SDL_free(ease_out_quint_transition);
	ease_out_quint_transition = NULL;
}
