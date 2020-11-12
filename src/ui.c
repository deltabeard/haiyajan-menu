/**
 * Renders UI for Haiyajan.
 * Copyright (c) 2020 Mahyar Koshkouei
 */

#include <menu.h>
#include <SDL.h>
#include <ui.h>

#if 0
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

struct elements
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

	struct elements *next;
};

struct ui_texture_cache
{
	void *hash;
	SDL_Texture *tex;
	struct ui_texture_cache *next;
};
#endif

struct ui_ctx
{
	/* Required to recreate texture on resizing. */
	SDL_Renderer *ren;

	/* Texture to render on. */
	SDL_Texture *tex;

	/* Whether the front-end must call ui_render_frame(). */
	SDL_bool redraw;

	/* Root Menu. */
	struct menu_ctx *root;
	/* Currently rendered menu. */
	struct menu_ctx *current;

	/* DPI that tex texture is rendered for. */
	float dpi;
};

/**
 * Ease out quint transition look up table. When transitioning from A to B,
 * the animation is defined as (B - A) * duration_ms.
*/
static double *ease_out_quint_transition = NULL;
/* The animation is for 800ms. */
static const unsigned ease_out_quint_duration_ms = 800;

void ui_input(ui_ctx *ui, SDL_GameControllerButton btn)
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

	SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Redraw requested");
	ui->redraw = SDL_TRUE;
	return;
}

SDL_bool ui_should_redraw(ui_ctx *c)
{
	return c->redraw;
}

int ui_render_frame(ui_ctx *c)
{
	int ret = 0;
	SDL_assert(c->tex != NULL);

	if(c->redraw == SDL_FALSE)
		goto out;

	ret = SDL_SetRenderTarget(c->ren, c->tex);

	SDL_SetRenderDrawColor(c->ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(c->ren);

	SDL_Rect main_menu_box = {150, 50, 100, 100};
	const unsigned box_spacing = 120;

	for(unsigned item = 0; item < c->current->items_u.static_list.items_nmemb; item++)
	{
		const SDL_Colour ol = c->current->items_u.static_list.items[item].selected_outline;
		const SDL_Colour bg = c->current->items_u.static_list.items[item].bg;

		if(item == c->current->item_selected)
		{
			SDL_Rect bg_box = main_menu_box;
			SDL_SetRenderDrawColor(c->ren, ol.r, ol.g, ol.b, ol.a);
			SDL_RenderDrawRect(c->ren, &bg_box);
			SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Selected %s",
				    c->current->items_u.static_list.items[item].name);
		}

		SDL_SetRenderDrawColor(c->ren, bg.r, bg.g, bg.b, bg.a);
		SDL_RenderFillRect(c->ren, &main_menu_box);
		main_menu_box.y += box_spacing;
	}

	ret = SDL_SetRenderTarget(c->ren, NULL);
	SDL_RenderCopy(c->ren, c->tex, NULL, NULL);

	SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "UI Rendered %s",
		    c->current->title);
	c->redraw = SDL_FALSE;

out:
	return ret;
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

struct ui_ctx *ui_init(SDL_Window *win, SDL_Renderer *ren, struct menu_ctx *root)
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

	c->root = root;
	c->current = root;
	c->redraw = SDL_TRUE;

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
