/**
 * Cache common components of UI.
 * Copyright (c) 2021-2022 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

#include "hedley.h"
#include "SDL.h"
#include "ui.h"
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

typedef enum {
	UI_TEXTURE_PART_LABEL,
	UI_TEXTURE_PART_ICON
} ui_texture_part_e;

typedef struct cache_ctx cache_ctx_s;

void dump_cache(cache_ctx_s *ctx, SDL_Renderer *r);

HEDLEY_NON_NULL(1,4)
SDL_Texture *get_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
	ui_texture_part_e part,
	Hash label_hash, const struct ui_element *HEDLEY_RESTRICT el);

HEDLEY_NON_NULL(1,4,5)
void store_cached_texture(cache_ctx_s *HEDLEY_RESTRICT ctx,
	ui_texture_part_e part,
	Hash label_hash, const struct ui_element *HEDLEY_RESTRICT el,
	SDL_Texture *HEDLEY_RESTRICT tex);

cache_ctx_s *init_cached_texture(void);

void deinit_cached_texture(cache_ctx_s *ctx);

void clear_cached_textures(cache_ctx_s *ctx);
