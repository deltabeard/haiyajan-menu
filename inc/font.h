/**
 * Font management for SDL2.
 * Copyright (C) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

#include <SDL.h>

typedef struct font_ctx font_ctx_s;

typedef enum
{
	FONT_QUALITY_LOW = 0,
	FONT_QUALITY_HIGH
} font_quality_e;

typedef enum
{
	FONT_STYLE_HEADER = 0,
	FONT_STYLE_ICON,
	FONT_STYLE_REGULAR,

	FONT_STYLE_MAX
} font_style_e;

SDL_Texture *font_render_icon(font_ctx_s *ctx, Uint16 icon, SDL_Colour fg);

SDL_Texture *font_render_text(font_ctx_s *ctx, const char *str,
	font_style_e s, font_quality_e q, SDL_Colour fg);

font_ctx_s *font_init(SDL_Renderer *rend, float dpi);
void font_exit(font_ctx_s *ctx);
