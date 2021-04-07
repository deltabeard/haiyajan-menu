/**
 * Font management for SDL2.
 * Copyright (C) 2020 Mahyar Koshkouei
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 */

#define DONT_HAVE_FRIBIDI_CONFIG_H
#define FRIBIDI_NO_DEPRECATED

#include <fonts/fabric-icons.h>
#include <fonts/NotoSansDisplay-Regular-Latin.h>
#include <fonts/NotoSansDisplay-SemiCondensedLight-Latin.h>

#include <font.h>
#include <fribidi.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <t1ha.h>

#if defined(__WIN32__)
# define WINDOWS_LEAN_AND_MEAN
# include <Windows.h>
#endif

/* Maximum number of fonts to preload. */
#define MAX_FONTS 8

struct font_ctx
{
	SDL_Renderer *rend;
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

	tex = SDL_CreateTextureFromSurface(ctx->rend, surf);
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
	size_t unicode_len = 0;

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
			for(unsigned i = 0; str[i] != '\0' && i < 4; i++)
			{
				unicode_len++;
			}

			for(unsigned i = 0; i < SDL_arraysize(ctx->ui_regular); i++)
			{
				Uint16 *unicode;
				int is_prov;
				TTF_Font *test;

				test = ctx->ui_regular[i];
				/* It is possible that the font is not available
				 * on the running platform. */
				if(test == NULL)
					continue;

				unicode = (Uint16 *)SDL_iconv_string("UCS-2-INTERNAL",
					"UTF-8", str, unicode_len + 1);
				is_prov = TTF_GlyphIsProvided(test, unicode[0]);
				SDL_free(unicode);

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
		size_t instrlen = SDL_strlen(str);
		FriBidiChar *instr, *outstr;
		FriBidiParType biditype = FRIBIDI_PAR_ON;
		FriBidiStrIndex strinlen;
		char *strutf8_out;

		instr = SDL_malloc(instrlen * sizeof(FriBidiChar));
		outstr = SDL_malloc(instrlen * sizeof(FriBidiChar));
		strinlen = fribidi_charset_to_unicode(FRIBIDI_CHAR_SET_UTF8, str, instrlen, instr);

		fribidi_log2vis(instr, strinlen, &biditype, outstr, NULL, NULL, NULL);
		SDL_free(instr);

		strutf8_out = SDL_malloc(instrlen);
		fribidi_unicode_to_charset(FRIBIDI_CHAR_SET_UTF8, outstr, strinlen, strutf8_out);
		SDL_free(outstr);

		SDL_Surface *surf;

		surf = TTF_Render_fn(font, strutf8_out, fg);
		SDL_free(strutf8_out);

		if(surf == NULL)
			goto out;

		ret = SDL_CreateTextureFromSurface(ctx->rend, surf);
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

/**
 * Initialise fonts given a DPI. This function always succeeds; a backup font is
 * used if a font cannot be found on the running platform.
 * 
 * \param ctx Font context.
 * \param dpi Requested DPI to scale fonts to.
*/
void font_change_pt(font_ctx_s *ctx, float dpi_multiply)
{
	int icon_size_ref = 72.0f * dpi_multiply;
	int header_size_ref = 40.0f * dpi_multiply;
	int regular_size_ref = 24.0f * dpi_multiply;

	font_close_ttf(ctx);

	/* Load built-in header font. */
	{
		SDL_RWops *hdr_font_mem;
		hdr_font_mem = SDL_RWFromConstMem(NotoSansDisplay_SemiCondensedLight_Latin_ttf,
			NotoSansDisplay_SemiCondensedLight_Latin_ttf_len);
		ctx->ui_header = TTF_OpenFontRW(hdr_font_mem, 1, header_size_ref);
	}

	/* Load built-in icon font. */
	{
		SDL_RWops *icon_font_mem;
		icon_font_mem = SDL_RWFromConstMem(fabric_icons_ttf,
			fabric_icons_ttf_len);
		ctx->ui_icons = TTF_OpenFontRW(icon_font_mem, 1, icon_size_ref);
	}

	/* Load built-in regular font in-case platform dependant fonts cannot
	 * be loaded below. */
	{
		SDL_RWops *font_mem;
		font_mem = SDL_RWFromConstMem(NotoSansDisplay_Regular_Latin_ttf,
			NotoSansDisplay_Regular_Latin_ttf_len);
		ctx->ui_regular[0] = TTF_OpenFontRW(font_mem, 1, regular_size_ref);
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
		ctx->ui_regular[s] = TTF_OpenFont(loc, regular_size_ref);
		if(ctx->ui_regular[s] == NULL)
			continue;

		/* If a font if successfully opened, move to next pointer. */
		s++;
	}

#elif defined(__LINUX__)
#elif defined(__FREEBSD__)
#elif defined(__MACOSX__)
#else
#endif

out:
	return;
}

font_ctx_s *font_init(SDL_Renderer *rend, float dpi_multiply)
{
	font_ctx_s *ctx = NULL;

	SDL_assert(rend != NULL);
	SDL_assert(dpi_multiply > 0.0f);

	if(!TTF_WasInit() && TTF_Init() == -1)
		goto out;

	ctx = SDL_calloc(1, sizeof(font_ctx_s));
	if(ctx == NULL)
		goto out;

	font_change_pt(ctx, dpi_multiply);
	ctx->rend = rend;

out:
	return ctx;
}

void font_exit(font_ctx_s *ctx)
{
	font_close_ttf(ctx);
	SDL_free(ctx);
}
