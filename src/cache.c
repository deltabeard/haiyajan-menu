/**
 * Cache common components of UI.
 * Copyright (c) 2021 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#include <cache.h>
#include <SDL.h>
#include <stretchy_buffer.h>

#include <wyhash.h>

#if SIZEOF_VOIDP == 8
typedef Uint64 Hash;
# define HASH_FN(dat, len, seed) wyhash64(dat, len, seed)
#else
typedef Uint32 Hash
# define HASH_FN(dat, len, seed) wyhash32(dat, len, seed)
#endif

struct textures {
	Hash hash;
	SDL_Texture *tex;
};

struct cache_ctx {
	struct textures *textures;
};

SDL_Texture *get_cached_texture(cache_ctx_s *ctx, const void *dat, size_t len)
{
	Hash hash;
	unsigned count;

	if(ctx == NULL)
		goto out;

	count = sb_count(ctx->textures);
	hash = HASH_FN(dat, len, 0);

	for(unsigned i = 0; i < count; i++)
	{
		if(hash != ctx->textures[i].hash)
			continue;

		return ctx->textures[i].tex;
	}

out:
	return NULL;
}

cache_ctx_s *store_cached_texture(cache_ctx_s *ctx, const void *dat, size_t len, SDL_Texture *tex)
{
	struct textures new_entry;

	if(ctx == NULL)
	{
		ctx = SDL_calloc(1, sizeof(struct cache_ctx));
		if(ctx == NULL)
			return NULL;
	}

	new_entry.hash = HASH_FN(dat, len, 0);
	new_entry.tex = tex;
	sb_push(ctx->textures, new_entry);

	return ctx;
}

void clear_cached_textures(cache_ctx_s *ctx)
{
	sb_free(ctx->textures);
}
