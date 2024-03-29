/**
 * Font management for SDL2.
 * Copyright (C) 2020-2022 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#include "fonts/fabric-icons.h"
#include "fonts/NotoSansDisplay-Regular-Latin.h"
#include "fonts/NotoSansDisplay-SemiCondensedLight-Latin.h"

#include "all.h"
#include "font.h"
#include "SDL.h"
#include "SDL_ttf.h"

#ifndef NO_FRIBIDI
# define DONT_HAVE_FRIBIDI_CONFIG_H
# define FRIBIDI_NO_DEPRECATED
# include <fribidi.h>
#endif

#if defined(__WIN32__)
# define WINDOWS_LEAN_AND_MEAN
# include <Windows.h>
#endif

/* Maximum number of fonts to preload. */
#define MAX_FONTS 8

struct font_ctx
{
	SDL_Renderer *rend;
	int tex_min_w, tex_min_h;
	TTF_Font *ui_header;
	TTF_Font *ui_icons;
	TTF_Font *ui_regular[MAX_FONTS];
};

SDL_Texture *font_render_icon(font_ctx_s *ctx, Uint16 icon, SDL_Colour fg)
{
	SDL_Surface *surf;
	SDL_Texture *tex = NULL;

	surf = TTF_RenderGlyph_Blended(ctx->ui_icons, icon, fg);
	if(surf == NULL)
		goto out;

	if(surf->w > ctx->tex_min_w || surf->h > ctx->tex_min_h)
	{
		SDL_LogError(HAIYAJAN_LOG_CATEGORY_FONT,
			       "Icon size (%ux%u) exceeds maximum texture size (%ux%u).",
			       surf->w, surf->h, ctx->tex_min_w, ctx->tex_min_h);
	}
	else
	{
		tex = SDL_CreateTextureFromSurface(ctx->rend, surf);
	}

	SDL_FreeSurface(surf);

out:
	return tex;
}

SDL_Texture *font_render_text(font_ctx_s *ctx, const char *str,
	font_style_e s, font_quality_e q, SDL_Colour fg)
{
	SDL_Texture *ret = NULL;
	TTF_Font *font = NULL;
	SDL_Surface *(*TTF_Render_fn)(TTF_Font *, const char *, SDL_Colour);

	SDL_assert(ctx != NULL);
	SDL_assert(str != NULL);

	switch(s)
	{
	case FONT_STYLE_HEADER:
		font = ctx->ui_header;
		break;

	case FONT_STYLE_ICON:
		font = ctx->ui_icons;
		break;

	case FONT_STYLE_REGULAR:
	default:
		for(unsigned i = 0; i < SDL_arraysize(ctx->ui_regular); i++)
		{
			int is_prov;
			TTF_Font *test;

			test = ctx->ui_regular[i];
			/* It is possible that the font is not available
			 * on the running platform. */
			if(test == NULL)
				continue;

			is_prov = TTF_GlyphIsProvided(test, str[0]);
			if(is_prov == 0)
				continue;

			font = test;
			break;
		}

		if(font == NULL)
		{
			/* Select any available font if unable to select a suitable one. */
			for(unsigned i = 0; i < SDL_arraysize(ctx->ui_regular); i++)
				font = ctx->ui_regular[i];
		}

		break;
	}

	/* If we still can't select a font, don't render any text. */
	if(font == NULL)
		goto out;

	if(q == FONT_QUALITY_LOW)
		TTF_Render_fn = TTF_RenderUTF8_Solid;
	else
		TTF_Render_fn = TTF_RenderUTF8_Blended;

	{
		/* Render text on tile. */
		SDL_Surface *surf;

#ifndef NO_FRIBIDI
		char *strutf8_out;
		FriBidiChar *instr, *outstr;
		FriBidiParType biditype = FRIBIDI_PAR_ON;
		FriBidiStrIndex strinlen;
		size_t instrlen = SDL_strlen(str);

		instr = SDL_malloc(instrlen * sizeof(FriBidiChar));
		if(instr == NULL)
			goto out;

		outstr = SDL_malloc(instrlen * sizeof(FriBidiChar));
		if(outstr == NULL)
		{
			free(instr);
			goto out;
		}

		strinlen = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, str,
				(FriBidiStrIndex)instrlen, instr);

		fribidi_log2vis(instr, strinlen, &biditype, outstr, NULL, NULL, NULL);
		SDL_free(instr);

		strutf8_out = SDL_malloc(instrlen);
		if(strutf8_out == NULL)
		{
			free(outstr);
			goto out;
		}

		fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, outstr, strinlen, strutf8_out);
		SDL_free(outstr);

		surf = TTF_Render_fn(font, strutf8_out, fg);
		SDL_free(strutf8_out);
#else
		surf = TTF_Render_fn(font, str, fg);
#endif

		if(surf == NULL)
			goto out;

		if(surf->w > ctx->tex_min_w || surf->h > ctx->tex_min_h)
		{
			SDL_LogError(HAIYAJAN_LOG_CATEGORY_FONT,
				     "Text size (%ux%u) exceeds maximum texture size (%ux%u).",
				     surf->w, surf->h, ctx->tex_min_w, ctx->tex_min_h);
		}
		else
		{
			ret = SDL_CreateTextureFromSurface(ctx->rend, surf);
		}

		SDL_FreeSurface(surf);
	}

out:
	return ret;
}

static void font_close_ttf(font_ctx_s *ctx)
{
	TTF_CloseFont(ctx->ui_header);
	TTF_CloseFont(ctx->ui_icons);

	ctx->ui_header = NULL;
	ctx->ui_icons = NULL;

	for(unsigned i = 0; i < MAX_FONTS; i++)
	{
		TTF_CloseFont(ctx->ui_regular[i]);
		ctx->ui_regular[i] = NULL;
	}
}

int font_get_height(font_ctx_s *ctx, font_style_e style)
{
	TTF_Font *f[] = {
		ctx->ui_regular[0], ctx->ui_header, ctx->ui_icons
	};

	SDL_assert(ctx != NULL);
	SDL_assert(style < FONT_STYLE_MAX);

	return TTF_FontHeight(f[style]);
}

/**
 * Initialise fonts given a DPI. This function always succeeds; a backup font is
 * used if a font cannot be found on the running platform.
 *
 * \param ctx Font context.
 * \param dpi Requested DPI to scale fonts to.
*/
void font_change_pt(font_ctx_s *ctx, unsigned hdpi, unsigned vdpi,
		int icon_pt, int header_pt, int regular_pt)
{
	SDL_assert(ctx->ui_icons != NULL);
	SDL_assert(ctx->ui_header != NULL);
	SDL_assert(ctx->ui_regular[0] != NULL);

	TTF_SetFontSizeDPI(ctx->ui_icons, icon_pt, hdpi, vdpi);
	TTF_SetFontSizeDPI(ctx->ui_header, header_pt, hdpi, vdpi);

	for(unsigned i = 0; i < MAX_FONTS; i++)
	{
		if(ctx->ui_regular[i] == NULL)
			break;

		TTF_SetFontSizeDPI(ctx->ui_regular[i], regular_pt, hdpi, vdpi);
	}
}

static void font_read_ttf(font_ctx_s *ctx)
{
	SDL_assert_paranoid(ctx->ui_icons == NULL);
	SDL_assert_paranoid(ctx->ui_header == NULL);
	SDL_assert_paranoid(ctx->ui_regular[0] == NULL);

	/* Load built-in header font. */
	{
		SDL_RWops *hdr_font_mem;
		hdr_font_mem = SDL_RWFromConstMem(NotoSansDisplay_SemiCondensedLight_Latin_ttf,
						  NotoSansDisplay_SemiCondensedLight_Latin_ttf_len);
		ctx->ui_header = TTF_OpenFontRW(hdr_font_mem, 1, 12);
	}

	/* Load built-in icon font. */
	{
		SDL_RWops *icon_font_mem;
		icon_font_mem = SDL_RWFromConstMem(fabric_icons_ttf,
						   fabric_icons_ttf_len);
		ctx->ui_icons = TTF_OpenFontRW(icon_font_mem, 1, 12);
	}

	/* Load built-in regular font in-case platform dependant fonts cannot
	 * be loaded below. */
	{
		SDL_RWops *font_mem;
		font_mem = SDL_RWFromConstMem(NotoSansDisplay_Regular_Latin_ttf,
					      NotoSansDisplay_Regular_Latin_ttf_len);
		ctx->ui_regular[0] = TTF_OpenFontRW(font_mem, 1, 12);
	}

#if defined(__WINDOWS__)
	char win[MAX_PATH];
	unsigned sz;
	char loc[2048];

	sz = GetWindowsDirectoryA(win, MAX_PATH);
	if(sz == 0)
		goto out;

	/* Initialise fonts from the given locations. */
	for(unsigned i = 0, s = 0; i < SDL_arraysize(ctx->ui_regular); i++)
	{
		const char *ui_regular_locs[MAX_FONTS] = {
			"SEGOEUI.TTF",	/* Latin */
			"ARIAL.TTF",	/* Latin (Fallback) */
			"MSYH.TTC",	/* Chinese (Sim.) */
			"MSGOTHIC.TTC",	/* Japanese */
			"MALGUN.TTF",	/* Korean */
			"NIRMALA.TTF",	/* Devanagari */
			"MSJH.TTF",	/* Chinese (Trad.) */
			"SEGUIEMJ.TTF"	/* Emoji */
		};

		sz = SDL_snprintf(loc, sizeof(loc), "%s\\FONTS\\%s", win,
			ui_regular_locs[i]);

		/* Errors are ignored. */
		ctx->ui_regular[s] = TTF_OpenFont(loc, 12);
		if(ctx->ui_regular[s] == NULL)
			continue;

		/* If a font is successfully opened, move to next pointer. */
		s++;
	}

#elif defined(__LINUX__)
#elif defined(__FREEBSD__)
#elif defined(__MACOSX__)
#else
#endif

	/* Suppresses unused goto on certain configurations. */
	goto out;
out:
	return;
}

font_ctx_s *font_init(SDL_Renderer *rend)
{
	font_ctx_s *ctx = NULL;

	SDL_assert(rend != NULL);

	if(!TTF_WasInit() && TTF_Init() == -1)
		goto out;

	ctx = SDL_calloc(1, sizeof(font_ctx_s));
	if(ctx == NULL)
		goto out;

	ctx->rend = rend;

	{
		SDL_RendererInfo rend_info;
		if(SDL_GetRendererInfo(rend, &rend_info) < 0)
		{
			/* Minimum guaranteed texture size is 64, which is
			 * unacceptable. Setting this to 1024, as supported by
			 * everything I've seen since year 2000.
			 * GeForce2 (2000) and Raspberry Pi 1 Model A (2011)
			 * both support 2048x2048 pixels. */
			ctx->tex_min_h = 1024;
			ctx->tex_min_w = 1024;
		}
		else
		{
			ctx->tex_min_h = rend_info.max_texture_height;
			ctx->tex_min_w = rend_info.max_texture_width;
		}

	}

	font_read_ttf(ctx);

out:
	return ctx;
}

void font_exit(font_ctx_s *ctx)
{
	font_close_ttf(ctx);
	SDL_free(ctx);
}
