/**
 * Cache common components of UI.
 * Copyright (c) 2021 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

#include <hedley.h>
#include <SDL.h>

typedef struct cache_ctx cache_ctx_s;

void dump_cache(cache_ctx_s *ctx, SDL_Renderer *r);

SDL_Texture *get_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
		const void *HEDLEY_RESTRICT dat, size_t len);

void store_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
		const void *HEDLEY_RESTRICT dat, size_t len,
		SDL_Texture *HEDLEY_RESTRICT tex);

cache_ctx_s *init_cached_texture(void);

void deinit_cached_texture(cache_ctx_s *ctx);

void clear_cached_textures(cache_ctx_s *ctx);
