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
#include "wyhash.h"

#if SIZEOF_VOIDP == 8
typedef Uint64 Hash;
# define HASH_FN(dat, len, seed) wyhash64(dat, len, seed)
# define PRIhashX "016" SDL_PRIX64
#else
typedef Uint32 Hash;
# define HASH_FN(dat, len, seed) wyhash32(dat, len, seed)
# define PRIhashX "08" SDL_PRIX32
#endif

struct textures {
	Hash hash;
	const void *reference;
	SDL_Texture *tex;
};

struct cache_ctx {
	struct textures *textures;
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
	count = stb_arr_len(ctx->textures);

	for(unsigned i = 0; i < count; i++)
	{
		struct textures *t;
		SDL_Surface *s;
		int out_len;
		qoi_desc qd;
		void *qoi_img;

		t = &ctx->textures[i];
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
				"%" PRIhashX ".qoi", t->hash);
			rw = SDL_RWFromFile(filename, "wb");
			SDL_RWwrite(rw, qoi_img, 1, out_len);
			SDL_RWclose(rw);
		}

		SDL_free(qoi_img);
	}
}

SDL_Texture *get_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
		const void *HEDLEY_RESTRICT key, size_t len)
{
	Hash hash;
	unsigned count;

	SDL_assert_paranoid(ctx != NULL);

	count = stb_arr_len(ctx->textures);
	hash = HASH_FN(key, len, 0);

	for(unsigned i = 0; i < count; i++)
	{
		if(hash != ctx->textures[i].hash)
			continue;

		return ctx->textures[i].tex;
	}

	return NULL;
}

void store_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
		const void *HEDLEY_RESTRICT dat, size_t len,
		const void *reference,
		SDL_Texture *HEDLEY_RESTRICT tex)
{
	struct textures new_entry;

	new_entry.hash = HASH_FN(dat, len, 0);
	new_entry.reference = reference;
	new_entry.tex = tex;
	/* FIXME: does not error on out of memory exception. */
	stb_arr_push(ctx->textures, new_entry);

	return;
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

	if(ctx->textures == NULL)
	{
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
				"Attempted to clear cache when cache wasn't "
				"initialised.");
		return;
	}

	count = stb_arr_len(ctx->textures);
	stb_arr_free(ctx->textures);
	ctx->textures = NULL;
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
		     "Cleared %d cached textures", count);
}
