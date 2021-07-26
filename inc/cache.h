/**
 * Cache common components of UI.
 * Copyright (c) 2021 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

#include <SDL.h>

typedef struct cache_ctx cache_ctx_s;

SDL_Texture *get_cached_texture(cache_ctx_s *ctx, const void *dat, Uint32 len);

cache_ctx_s *store_cached_texture(cache_ctx_s *ctx, const void *dat, Uint32 len, SDL_Texture *tex);

void clear_cached_textures(cache_ctx_s *ctx);
