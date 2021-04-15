/**
 * Font management for SDL2.
 * Copyright (C) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#pragma once

/* Required for SDL_Colour */
#include <SDL.h>

/**
 * Opaque font context.
 */
typedef struct font_ctx font_ctx_s;

/* Low quality is fast but ugly. */
typedef enum
{
	FONT_QUALITY_LOW = 0,
	FONT_QUALITY_HIGH
} font_quality_e;

/**
 * Header: Thin, Semi-Compressed, large sans-serif font.
 * Icon: Fabric Icon font, large.
 * Regular: Regular sans-serif font for normal text.
 */
typedef enum
{
	FONT_STYLE_HEADER = 0,
	FONT_STYLE_ICON,
	FONT_STYLE_REGULAR,

	FONT_STYLE_MAX
} font_style_e;

/**
 * Renders a single UTF-16 glyph with the embedded Fabric UI font.
 * The rendering is slow but high quality, so should be cached by the user.
 * 
 * \param ctx	Font context.
 * \param icon	UTF-16 glyph.
 * \param fg	Colour of glyph to render.
 * \return	Rendered glyph in texture, or NULL on error (check SDL_GetError()).
*/
SDL_Texture *font_render_icon(font_ctx_s *ctx, Uint16 icon, SDL_Colour fg);

/**
 * Renders the UTF-8 string str given the font style s, rendering quality q,
 * and the font colour fg.
 * 
 * \param ctx	Font context.
 * \param str	UTF-8 string.
 * \param s	Style of font.
 * \param q	Quality of rendering. Low quality is fast but ugly.
 * \param fg	Font colour.
 * \return	Rendered string in texture, or NULL on error (check SDL_GetError()).
*/
SDL_Texture *font_render_text(font_ctx_s *ctx, const char *str,
	font_style_e s, font_quality_e q, SDL_Colour fg);

/**
 * Scale font sizes depending on the DPI scale.
 * Internal font sizes are referenced to a DPI of 96.
 *
 * \param ctx		Font context.
*/
void font_change_pt(font_ctx_s *restrict ctx,
	int icon_pt, int header_pt, int regular_pt);

/**
 * Initialise a new font context.
 * 
 * \param rend		SDL Renderer
 * \return		Font context or NULL on error (check SDL_GetError()).
*/
font_ctx_s *font_init(SDL_Renderer *rend,
	int icon_pt, int header_pt, int regular_pt);

/**
 * Frees a font context.
 * 
 * \param ctx	Font context to free.
 */
void font_exit(font_ctx_s *ctx);
