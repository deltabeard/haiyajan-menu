/**
 * Cache common components of UI.
 * Copyright (c) 2021-2022 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#define QOI_MALLOC(sz)	SDL_malloc(sz)
#define QOI_FREE(p)	SDL_free(p)
#define QOI_ZEROARR(a)	SDL_memset((a),0,sizeof(a))

#include "cache.h"
#include "hedley.h"
#include "qoi.h"
#include "SDL.h"
#define STB_LIB_IMPLEMENTATION
#include "stb_arr.h"
#include "ui.h"

struct textures {
	ui_texture_part_e part;
	Hash label_hash;
	const void *data_origin;
	struct ui_element el;
	SDL_Texture *tex;
};

struct cache_ctx {
	struct textures *cached_ui;
};

static const char *part_str[] = {
	"label", "icon"
};

static SDL_Surface *tex_to_surf(SDL_Renderer *rend, SDL_Texture *tex)
{
	SDL_Surface *surf = NULL;
	SDL_Texture *core_tex;
	SDL_Rect rect = { 0 };
	int fmt = SDL_PIXELFORMAT_ARGB8888;

	SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h);
	core_tex = SDL_CreateTexture(rend, fmt, SDL_TEXTUREACCESS_TARGET,
				     rect.w, rect.h);
	if(core_tex == NULL)
		return NULL;

	if(SDL_SetRenderTarget(rend, core_tex) != 0)
		goto err;

	SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
	SDL_RenderClear(rend);

	if(SDL_RenderCopy(rend, tex, NULL, &rect) != 0)
		goto err;

	surf = SDL_CreateRGBSurfaceWithFormat(0, rect.w, rect.h,
					      SDL_BITSPERPIXEL(fmt), fmt);
	if(surf == NULL)
		goto err;

	if(SDL_RenderReadPixels(rend, &rect, fmt, surf->pixels, surf->pitch) != 0)
	{
		SDL_FreeSurface(surf);
		goto err;
	}

err:
	SDL_SetRenderTarget(rend, NULL);
	SDL_DestroyTexture(core_tex);
	return surf;
}

void dump_cache(cache_ctx_s *ctx, SDL_Renderer *rend)
{
	unsigned count;
	count = stb_arr_len(ctx->cached_ui);

	for(unsigned i = 0; i < count; i++)
	{
		struct textures *t;
		SDL_Surface *s;
		int out_len;
		qoi_desc qd;
		void *qoi_img;

		t = &ctx->cached_ui[i];
		s = tex_to_surf(rend, t->tex);
		if(s == NULL)
		{
			char errstr[128];
			SDL_GetErrorMsg(errstr, sizeof(errstr));
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				     "Unable to convert texture to surface for "
				     "cache dump: %s",
				     errstr);
			continue;
		}

		SDL_LockSurface(s);
		qd.width = s->w;
		qd.height = s->h;
		qd.colorspace = QOI_LINEAR;
		qd.channels = SDL_BYTESPERPIXEL(s->format->format);
		qoi_img = qoi_encode(s->pixels, &qd, &out_len);
		SDL_UnlockSurface(s);
		SDL_FreeSurface(s);

		{
			SDL_RWops *rw;
			char filename[32];

			SDL_snprintf(filename, sizeof(filename),
				"%" PRIhashX ".qoi", t->label_hash);
			rw = SDL_RWFromFile(filename, "wb");
			SDL_RWwrite(rw, qoi_img, 1, out_len);
			SDL_RWclose(rw);
		}

		SDL_free(qoi_img);
	}
}

static void delete_cached_texture_loc(cache_ctx_s *HEDLEY_RESTRICT ctx,
	unsigned loc)
{
	SDL_LogDebug(HAIYAJAN_LOG_CATEGORY_CACHE,
		"Deleting texture at location %u", loc);
	stb_arr_fastdelete(ctx->cached_ui, loc);
}

HEDLEY_NON_NULL(1,4)
SDL_Texture *get_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
	ui_texture_part_e part,
	Hash label_hash, const struct ui_element *HEDLEY_RESTRICT el)
{
	unsigned count;

	SDL_assert_paranoid(el->label != NULL);
	SDL_LogDebug(HAIYAJAN_LOG_CATEGORY_CACHE,
		"Looking up %s texture for label '%s' ( %" PRIhashX " %p)",
		part_str[part], el->label, label_hash, (void *)el);

	count = stb_arr_len(ctx->cached_ui);

	for(unsigned i = 0; i < count; i++)
	{
		if(el != ctx->cached_ui[i].data_origin)
			continue;

		if(part != ctx->cached_ui[i].part)
			continue;

		if(label_hash != ctx->cached_ui[i].label_hash)
		{
			SDL_LogDebug(HAIYAJAN_LOG_CATEGORY_CACHE,
				"Found %s texture for %p at location %u, "
				"but label hash changed from %" PRIhashX " "
				"to %" PRIhashX,
				part_str[part], (void *)el, i,
				ctx->cached_ui[i].label_hash, label_hash);
			delete_cached_texture_loc(ctx, i);
			break;
		}

		/* TODO: Check for any differences in the two elements. */

		SDL_LogDebug(HAIYAJAN_LOG_CATEGORY_CACHE,
			"Successfully found %s texture for %p",
			part_str[part], (void *)el);
		return ctx->cached_ui[i].tex;
	}

	SDL_LogDebug(HAIYAJAN_LOG_CATEGORY_CACHE,
		"No texture found for %" PRIhashX " %p",
		label_hash, (void *)el);

	return NULL;
}

HEDLEY_NON_NULL(1,4,5)
void store_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
		ui_texture_part_e part,
		Hash label_hash, const struct ui_element *HEDLEY_RESTRICT el,
		SDL_Texture *HEDLEY_RESTRICT tex)
{
	struct textures new_entry;

	SDL_assert_paranoid(el->label != NULL);

	/* Part of the UI element that the texture represents. */
	new_entry.part = part;
	/* A hash of the label. If this is a label of a dynamic element, then
	 * the seed is the member number of that label within the dynamic
	 * array. If this hash is different when fetching the texture from
	 * cache, then we know that the cached texture is stale. */
	new_entry.label_hash = label_hash;
	/* Pointer to where the ui_element data is held. Used as a reference
	 * to find out whether data from the same UI element has changed or
	 * not. */
	new_entry.data_origin = el;
	/* Holding a copy of the element data to compare with the element
	 * data when fetching the texture from cache. If any of this data has
	 * changed, then we know that the cached texture is stale. This is
	 * useful in case the label doesn't change, but other parameters of
	 * the element do. */
	SDL_memcpy(&new_entry.el, el, sizeof(*el));
	/* The rendered texture to store into the cache. */
	new_entry.tex = tex;

	SDL_LogDebug(HAIYAJAN_LOG_CATEGORY_CACHE,
		"Stored %s texture: '%s' (%" PRIhashX " %p)",
		part_str[part], el->label, label_hash, (void *)el);

	/* FIXME: does not error on out of memory exception. */
	stb_arr_push(ctx->cached_ui, new_entry);
}

cache_ctx_s *init_cached_texture(void)
{
	cache_ctx_s *ctx;
	ctx = SDL_calloc(1, sizeof(struct cache_ctx));
	return ctx;
}

void deinit_cached_texture(cache_ctx_s *ctx)
{
	SDL_assert_paranoid(ctx != NULL);
	SDL_free(ctx);
	ctx = NULL;
}

void clear_cached_textures(cache_ctx_s *ctx)
{
	unsigned count;

	if(ctx->cached_ui == NULL)
	{
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
				"Attempted to clear cache when cache wasn't "
				"initialised.");
		return;
	}

	count = stb_arr_len(ctx->cached_ui);
	stb_arr_free(ctx->cached_ui);
	ctx->cached_ui = NULL;
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
		     "Cleared %d cached textures", count);
}
