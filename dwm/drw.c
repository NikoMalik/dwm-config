/* See LICENSE file for copyright and license details. */
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <stdint.h>

#include <smmintrin.h>

#include <stdint.h>
#include <x86intrin.h>
#include <immintrin.h>

#include "drw.h"
#include "util.h"
#define UTF_INVALID 0xFFFD

#define UTF8_BATCH_SIZE 32

// static inline int utf8decode(const char *s_in, long *u, int *err) {
//     static const unsigned char lens[] = {
//         /* 0XXXX */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//         /* 10XXX */ 0, 0, 0, 0, 0, 0, 0, 0, /* invalid */
//         /* 110XX */ 2, 2, 2, 2,
//         /* 1110X */ 3, 3,
//         /* 11110 */ 4,
//         /* 11111 */ 0, /* invalid */
//     };
//     static const unsigned char leading_mask[] = {0x7F, 0x1F, 0x0F, 0x07};
//     static const unsigned int overlong[] = {0x0, 0x80, 0x0800, 0x10000};

//     const unsigned char *s = (const unsigned char *)s_in;
//     int len = lens[*s >> 3];

//     *u = UTF_INVALID;
//     *err = 1;
//     if (len == 0)
//         return 1;

//     long cp = s[0] & leading_mask[len - 1];
//     for (int i = 1; i < len; ++i) {
//         if (s[i] == '\0' || (s[i] & 0xC0) != 0x80)
//             return i;
//         cp = (cp << 6) | (s[i] & 0x3F);
//     }
//     /* out of range, surrogate, overlong encoding */
//     if (cp > 0x10FFFF || (cp >> 11) == 0x1B || cp < overlong[len - 1])
//         return len;

//     *err = 0;
//     *u = cp;
//     return len;
// }

inline __m256i push_last_byte_of_a_to_b(__m256i a, __m256i b) {
    return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 15);
}

inline __m256i push_last_2bytes_of_a_to_b(__m256i a, __m256i b) {
    return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 14);
}

inline __m256i push_last_3bytes_of_a_to_b(__m256i a, __m256i b) {
    return _mm256_alignr_epi8(b, _mm256_permute2x128_si256(a, b, 0x21), 13);
}

inline int utf8_decode_batch_avx2(const unsigned char *s, int *lens, long *cps) {
    __m256i input = _mm256_loadu_si256((const __m256i *)s);

    __m256i hi_nibbles = _mm256_srli_epi16(input, 4);
    __m256i len_mask = _mm256_shuffle_epi8(_mm256_setr_epi8(
                                               1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4,
                                               1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4),
                                           hi_nibbles);

    __m256i cont_mask = _mm256_cmpgt_epi8(_mm256_set1_epi8(-0x41), input);
    __m256i errors = _mm256_andnot_si256(cont_mask, len_mask);

    _mm256_storeu_si256((__m256i *)lens, len_mask);
    _mm256_storeu_si256((__m256i *)cps, errors);

    return _mm256_testz_si256(errors, errors);
}

static int utf8decode(const char *s_in, long *u, int *err) {
    static const unsigned char lens[] = {
        /* 0XXXX */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 10XXX */ 0, 0, 0, 0, 0, 0, 0, 0, /* invalid */
        /* 110XX */ 2, 2, 2, 2,
        /* 1110X */ 3, 3,
        /* 11110 */ 4,
        /* 11111 */ 0, /* invalid */
    };
    static const unsigned char leading_mask[] = {0x7F, 0x1F, 0x0F, 0x07};
    static const unsigned int overlong[] = {0x0, 0x80, 0x800, 0x10000};

    const unsigned char *s = (const unsigned char *)s_in;
    int len = lens[*s >> 3];

    *u = UTF_INVALID;
    *err = 1;

    if (len == 0 || len > 4) {
        return 1;
    }

    long cp = s[0] & leading_mask[len - 1];

    for (int i = 1; i < len; ++i) {
        if (s[i] == '\0' || (s[i] & 0xC0) != 0x80) {
            return i;
        }
        cp = (cp << 6) | (s[i] & 0x3F);
    }

    if (cp > 0x10FFFF ||
        (cp >= 0xD800 && cp <= 0xDFFF) ||
        cp < overlong[len - 1]) {
        return len;
    }
    *u = cp;
    *err = 0;
    return len;
}

typedef union {
    __m256i v;
    uint8_t b[32];
    uint32_t u32[8];
} simd_reg;

static inline void utf8_avx2_decode(const uint8_t *src, uint32_t *codepoints, int *lengths) {
    simd_reg input = {.v = _mm256_loadu_si256((const __m256i *)src)};

    const __m256i len_mask = _mm256_setr_epi8(
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4);
    simd_reg len = {.v = _mm256_shuffle_epi8(len_mask, _mm256_srli_epi16(input.v, 4))};

    __m256i cont_mask = _mm256_set1_epi8(0xC0);
    __m256i next1 = _mm256_alignr_epi8(input.v, input.v, 1);
    __m256i next2 = _mm256_alignr_epi8(input.v, input.v, 2);
    __m256i next3 = _mm256_alignr_epi8(input.v, input.v, 3);
    __m256i cont_check = _mm256_and_si256(
        _mm256_or_si256(
            _mm256_or_si256(
                _mm256_cmpeq_epi8(_mm256_and_si256(next1, cont_mask), _mm256_set1_epi8(0x80)),
                _mm256_cmpeq_epi8(len.v, _mm256_set1_epi8(1))),
            _mm256_or_si256(
                _mm256_cmpeq_epi8(_mm256_and_si256(next2, cont_mask), _mm256_set1_epi8(0x80)),
                _mm256_cmpeq_epi8(len.v, _mm256_set1_epi8(2)))),
        _mm256_cmpeq_epi8(_mm256_and_si256(next3, cont_mask), _mm256_set1_epi8(0x80)));

    __m256i mask1 = _mm256_set1_epi8(0x3F);
    __m256i mask2 = _mm256_set1_epi8(0x1F);
    __m256i mask3 = _mm256_set1_epi8(0x0F);
    __m256i mask4 = _mm256_set1_epi8(0x07);

    __m256i cp1 = input.v;
    __m256i cp2 = _mm256_or_si256(
        _mm256_slli_epi16(_mm256_and_si256(input.v, mask2), 6),
        _mm256_and_si256(next1, mask1));
    __m256i cp3 = _mm256_or_si256(
        _mm256_slli_epi16(_mm256_and_si256(input.v, mask3), 12),
        _mm256_or_si256(
            _mm256_slli_epi16(_mm256_and_si256(next1, mask1), 6),
            _mm256_and_si256(next2, mask1)));
    __m256i cp4 = _mm256_or_si256(
        _mm256_slli_epi16(_mm256_and_si256(input.v, mask4), 18),
        _mm256_or_si256(
            _mm256_slli_epi16(_mm256_and_si256(next1, mask1), 12),
            _mm256_or_si256(
                _mm256_slli_epi16(_mm256_and_si256(next2, mask1), 6),
                _mm256_and_si256(next3, mask1))));

    __m256i final_cp = _mm256_blendv_epi8(
        _mm256_blendv_epi8(
            _mm256_blendv_epi8(cp1, cp2, _mm256_cmpeq_epi8(len.v, _mm256_set1_epi8(2))),
            cp3, _mm256_cmpeq_epi8(len.v, _mm256_set1_epi8(3))),
        cp4, _mm256_cmpeq_epi8(len.v, _mm256_set1_epi8(4)));

    // __m256i valid = _mm256_andnot_si256(
    //     _mm256_or_si256(
    //         _mm256_cmpgt_epi8(len.v, _mm256_set1_epi8(4)),
    //         _mm256_or_si256(
    //             _mm256_cmpgt_epi32(final_cp, _mm256_set1_epi32(0x10FFFF)),
    //             _mm256_and_si256(
    //                 _mm256_cmpgt_epi32(final_cp, _mm256_set1_epi32(0xD7FF)),
    //                 _mm256_cmpgt_epi32(final_cp, _mm256_set1_epi32(0xE000))))),
    //     _mm256_and_si256(cont_check, _mm256_set1_epi8(0xFF)));

    _mm256_storeu_si256((__m256i *)codepoints, final_cp);
    _mm256_storeu_si256((__m256i *)lengths, len.v);
}

// static inline int utf8decode(const char *s_in, long *u, int *err) {
//     const unsigned char *s = (const unsigned char *)s_in;
//     unsigned char first_byte = s[0];
//     int len;

//     *err = 1;
//     *u = UTF_INVALID;

//     for (int i = 1; i < len; i++) {
//         if (s[i] == '\0' || s[i] < 0x80 || s[i] > 0xBF) {
//             return i;
//         }
//     }

//     long cp = first_byte & (0xFF >> len);
//     for (int i = 1; i < len; i++) {
//         cp = (cp << 6) | (s[i] & 0x3F);
//     }

//     static const unsigned int overlong[] = {0x0, 0x80, 0x800, 0x10000};
//     if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) || cp < overlong[len - 1]) {
//         return len;
//     }

//     *u = cp;
//     *err = 0;
//     return len;
// }

Drw *drw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
// drw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h, Visual *visual, unsigned int depth, Colormap cmap)

{

    Drw *drw = ecalloc(1, sizeof(Drw));

    drw->dpy = dpy;

    drw->screen = screen;

    drw->root = root;

    drw->w = w;

    drw->h = h;

    drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));

    drw->gc = XCreateGC(dpy, root, 0, NULL);

    XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

    return drw;
}
void drw_free(Drw *drw)

{

    XFreePixmap(drw->dpy, drw->drawable);

    XFreeGC(drw->dpy, drw->gc);

    drw_fontset_free(drw->fonts);

    free(drw);
}

/* This function is an implementation detail. Library users should use
 * drw_fontset_create instead.
 */
static Fnt *
xfont_create(Drw *drw, const char *fontname, FcPattern *fontpattern)

{

    Fnt *font;

    XftFont *xfont = NULL;

    FcPattern *pattern = NULL;

    if (fontname) {

        /* Using the pattern found at font->xfont->pattern does not yield the


         * same substitution results as using the pattern returned by


         * FcNameParse; using the latter results in the desired fallback


         * behaviour whereas the former just results in missing-character


         * rectangles being drawn, at least with some fonts. */

        if (!(xfont = XftFontOpenName(drw->dpy, drw->screen, fontname))) {
#ifdef __DEBUG__

            fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
#endif

            return NULL;
        }

        if (!(pattern = FcNameParse((FcChar8 *)fontname))) {

#ifdef __DEBUG__

            fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n", fontname);
#endif

            XftFontClose(drw->dpy, xfont);

            return NULL;
        }

    } else if (fontpattern) {

        if (!(xfont = XftFontOpenPattern(drw->dpy, fontpattern))) {

#ifdef __DEBUG__

            fprintf(stderr, "error, cannot load font from pattern.\n");

#endif

            return NULL;
        }

    } else {

        die("no font specified.");
    }

    font = ecalloc(1, sizeof(Fnt));

    font->xfont = xfont;

    font->pattern = pattern;

    font->h = xfont->ascent + xfont->descent;

    font->dpy = drw->dpy;

    return font;
}

static void
// xfont_free(Fnt *font) {
//     if (!font)
//         return;
//     if (font->pattern)
//         FcPatternDestroy(font->pattern);
//     XftFontClose(font->dpy, font->xfont);
//     free(font);
// }
xfont_free(Fnt *font)

{

    if (!font)

        return;

    if (font->pattern)

        FcPatternDestroy(font->pattern);

    XftFontClose(font->dpy, font->xfont);

    free(font);
}

// Fnt *drw_fontset_create(Drw *drw, const char *fonts[], size_t fontcount) {
//     Fnt *cur, *ret = NULL;
//     size_t i;

//     if (!drw || !fonts)
//         return NULL;

//     for (i = 1; i <= fontcount; i++) {
//         if ((cur = xfont_create(drw, fonts[fontcount - i], NULL))) {
//             cur->next = ret;
//             ret = cur;
//         }
//     }
//     return (drw->fonts = ret);
// }

Fnt *

drw_fontset_create(Drw *drw, const char *fonts[], size_t fontcount)

{

    Fnt *cur, *ret = NULL;

    size_t i;

    if (!drw || !fonts)

        return NULL;

    for (i = 1; i <= fontcount; i++) {

        if ((cur = xfont_create(drw, fonts[fontcount - i], NULL))) {

            cur->next = ret;

            ret = cur;
        }
    }

    return (drw->fonts = ret);
}

void drw_fontset_free(Fnt *font) {
    if (font) {
        drw_fontset_free(font->next);
        xfont_free(font);
    }
}

void drw_clr_create(Drw *drw, Clr *dest, const char *clrname)

{

    if (!drw || !dest || !clrname)

        return;

    if (!XftColorAllocName(drw->dpy, DefaultVisual(drw->dpy, drw->screen),

                           DefaultColormap(drw->dpy, drw->screen),

                           clrname, dest))

        die("error, cannot allocate color '%s'", clrname);
}

/* Wrapper to create color schemes. The caller has to call free(3) on the
 * returned color scheme when done using it. */

Clr *

drw_scm_create(Drw *drw, const char *clrnames[], size_t clrcount)

{

    size_t i;

    Clr *ret;

    /* need at least two colors for a scheme */

    if (!drw || !clrnames || clrcount < 2 || !(ret = ecalloc(clrcount, sizeof(XftColor))))

        return NULL;

    for (i = 0; i < clrcount; i++)

        drw_clr_create(drw, &ret[i], clrnames[i]);

    return ret;
}

void drw_setscheme(Drw *drw, Clr *scm) {
    if (drw)
        drw->scheme = scm;
}

void

drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert)

{

    if (!drw || !drw->scheme)

        return;

    XSetForeground(drw->dpy, drw->gc, invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);

    if (filled)

        XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);

    else

        XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}

int drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert) {

    int ty, ellipsis_x = 0;

    unsigned int tmpw, ew, ellipsis_w = 0, ellipsis_len, hash, h0, h1;

    XftDraw *d = NULL;

    Fnt *usedfont, *curfont, *nextfont;

    int utf8strlen, utf8charlen, utf8err, render = x || y || w || h;

    long utf8codepoint = 0;

    const char *utf8str;

    FcCharSet *fccharset;

    FcPattern *fcpattern;

    FcPattern *match;

    XftResult result;

    int charexists = 0, overflow = 0;

    /* keep track of a couple codepoints for which we have no match. */

    static unsigned int nomatches[128], ellipsis_width, invalid_width;

    static const char invalid[] = "�";

    if (!drw || (render && (!drw->scheme || !w)) || !text || !drw->fonts)

        return 0;

    if (!render) {

        w = invert ? invert : ~invert;

    } else {

        XSetForeground(drw->dpy, drw->gc, drw->scheme[invert ? ColFg : ColBg].pixel);

        XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);

        if (w < lpad)

            return x + w;

        d = XftDrawCreate(drw->dpy, drw->drawable,

                          DefaultVisual(drw->dpy, drw->screen),

                          DefaultColormap(drw->dpy, drw->screen));

        x += lpad;

        w -= lpad;
    }

    usedfont = drw->fonts;

    if (!ellipsis_width && render)

        ellipsis_width = drw_fontset_getwidth(drw, "...");

    if (!invalid_width && render)

        invalid_width = drw_fontset_getwidth(drw, invalid);

    while (1) {

        ew = ellipsis_len = utf8err = utf8charlen = utf8strlen = 0;

        utf8str = text;

        nextfont = NULL;

        while (*text) {

            utf8charlen = utf8decode(text, &utf8codepoint, &utf8err);

            for (curfont = drw->fonts; curfont; curfont = curfont->next) {

                charexists = charexists || XftCharExists(drw->dpy, curfont->xfont, utf8codepoint);

                if (charexists) {

                    drw_font_getexts(curfont, text, utf8charlen, &tmpw, NULL);

                    if (ew + ellipsis_width <= w) {

                        /* keep track where the ellipsis still fits */

                        ellipsis_x = x + ew;

                        ellipsis_w = w - ew;

                        ellipsis_len = utf8strlen;
                    }

                    if (ew + tmpw > w) {

                        overflow = 1;

                        /* called from drw_fontset_getwidth_clamp():


                         * it wants the width AFTER the overflow


                         */

                        if (!render)

                            x += tmpw;

                        else

                            utf8strlen = ellipsis_len;

                    } else if (curfont == usedfont) {

                        text += utf8charlen;

                        utf8strlen += utf8err ? 0 : utf8charlen;

                        ew += utf8err ? 0 : tmpw;

                    } else {

                        nextfont = curfont;
                    }

                    break;
                }
            }

            if (overflow || !charexists || nextfont || utf8err)

                break;

            else

                charexists = 0;
        }

        if (utf8strlen) {

            if (render) {

                ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent;

                XftDrawStringUtf8(d, &drw->scheme[invert ? ColBg : ColFg],

                                  usedfont->xfont, x, ty, (XftChar8 *)utf8str, utf8strlen);
            }

            x += ew;

            w -= ew;
        }

        if (utf8err && (!render || invalid_width < w)) {

            if (render)

                drw_text(drw, x, y, w, h, 0, invalid, invert);

            x += invalid_width;

            w -= invalid_width;
        }

        if (render && overflow)

            drw_text(drw, ellipsis_x, y, ellipsis_w, h, 0, "...", invert);

        if (!*text || overflow) {

            break;

        } else if (nextfont) {

            charexists = 0;

            usedfont = nextfont;

        } else {

            /* Regardless of whether or not a fallback font is found, the


             * character must be drawn. */

            charexists = 1;

            hash = (unsigned int)utf8codepoint;

            hash = ((hash >> 16) ^ hash) * 0x21F0AAAD;

            hash = ((hash >> 15) ^ hash) * 0xD35A2D97;

            h0 = ((hash >> 15) ^ hash) % LENGTH(nomatches);

            h1 = (hash >> 17) % LENGTH(nomatches);

            /* avoid expensive XftFontMatch call when we know we won't find a match */

            if (nomatches[h0] == utf8codepoint || nomatches[h1] == utf8codepoint)

                goto no_match;

            fccharset = FcCharSetCreate();

            FcCharSetAddChar(fccharset, utf8codepoint);

            if (!drw->fonts->pattern) {

                /* Refer to the comment in xfont_create for more information. */

                die("the first font in the cache must be loaded from a font string.");
            }

            fcpattern = FcPatternDuplicate(drw->fonts->pattern);

            FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);

            FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);

            FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);

            FcDefaultSubstitute(fcpattern);

            match = XftFontMatch(drw->dpy, drw->screen, fcpattern, &result);

            FcCharSetDestroy(fccharset);

            FcPatternDestroy(fcpattern);

            if (match) {

                usedfont = xfont_create(drw, NULL, match);

                if (usedfont && XftCharExists(drw->dpy, usedfont->xfont, utf8codepoint)) {

                    for (curfont = drw->fonts; curfont->next; curfont = curfont->next)

                        ; /* NOP */

                    curfont->next = usedfont;

                } else {

                    xfont_free(usedfont);

                    nomatches[nomatches[h0] ? h1 : h0] = utf8codepoint;

                no_match:

                    usedfont = drw->fonts;
                }
            }
        }
    }

    if (d)

        XftDrawDestroy(d);

    return x + (render ? w : 0);
}

void drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h) {
    if (!drw)
        return;

    XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
    XSync(drw->dpy, False);
}

#define FONT_CACHE_SIZE 256

typedef struct {
    FcChar32 codepoint; // .UTF-8
    Fnt *font;          // *font
} FontCacheEntry;

static FontCacheEntry font_cache[FONT_CACHE_SIZE] = {0};

static void reset_font_cache(void) {
    memset(font_cache, 0, sizeof(font_cache));
}

void drw_setfontset(Drw *drw, Fnt *set) {
    if (drw) {
        drw->fonts = set;
        reset_font_cache();
    }
}

// void drw_resize(Drw *drw, unsigned int w, unsigned int h) {

//     if (!drw)

//         return;

//     drw->w = w;

//     drw->h = h;

//     if (drw->drawable)

//         XFreePixmap(drw->dpy, drw->drawable);

//     drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
//     reset_font_cache();
// }

void drw_resize(Drw *drw, unsigned int w, unsigned int h) {
    if (!drw)
        return;
    drw->w = w;
    drw->h = h;
    if (drw->drawable)
        XFreePixmap(drw->dpy, drw->drawable);
    drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
    reset_font_cache();
}

static inline uint32_t crc32_hash(FcChar32 codepoint) {
    return _mm_crc32_u32(0, codepoint);
}

// static Fnt *find_font_for_char(Drw *drw, FcChar32 codepoint) {
//     if (!drw || !drw->fonts) {
//         return NULL;
//     }
//     // check our trashbank
//     //
//     // unsigned int hash = (unsigned int)codepoint;
//     // hash = ((hash >> 16) ^ hash) * 0x21F0AAAD;
//     // hash = ((hash >> 15) ^ hash) * 0xD35A2D97;
//     // unsigned int index = hash % FONT_CACHE_SIZE;
//     //
//     // uint32_t hash = crc32_hash(codepoint) % FONT_CACHE_SIZE;
//     // if (font_cache[hash].codepoint == codepoint) {
//     //     return font_cache[hash].font;
//     // }
//     // for (int i = 0; i < FONT_CACHE_SIZE; i++) {
//     //     if (font_cache[i].codepoint == codepoint && font_cache[i].font) {
//     //         return font_cache[i].font;
//     //     }
//     // }

//     // find right font
//     // for (Fnt *curfont = drw->fonts; curfont; curfont = curfont->next) {
//     //     if (XftCharExists(drw->dpy, curfont->xfont, codepoint)) {
//     //         // to cache
//     //         static int cache_index = 0;
//     //         font_cache[cache_index].codepoint = codepoint;
//     //         font_cache[cache_index].font = curfont;
//     //         cache_index = (cache_index + 1) % FONT_CACHE_SIZE;
//     //         return curfont;
//     //     }
//     // }

//     uint32_t hash = crc32_hash(codepoint) % FONT_CACHE_SIZE;
//     if (font_cache[hash].codepoint == codepoint) {
//         return font_cache[hash].font;
//     }
//     for (Fnt *curfont = drw->fonts; curfont; curfont = curfont->next) {
//         if (XftCharExists(drw->dpy, curfont->xfont, codepoint)) {
//             font_cache[hash].codepoint = codepoint;
//             font_cache[hash].font = curfont;
//             return curfont;
//         }
//     }
//     return drw->fonts;
// }

static Fnt *find_font_for_char(Drw *drw, FcChar32 codepoint) {
    if (!drw || !drw->fonts)
        return NULL;
    uint32_t hash = crc32_hash(codepoint) % FONT_CACHE_SIZE;
    if (font_cache[hash].codepoint == codepoint) {
        return font_cache[hash].font;
    }
    for (Fnt *curfont = drw->fonts; curfont; curfont = curfont->next) {
        if (XftCharExists(drw->dpy, curfont->xfont, codepoint)) {
            font_cache[hash].codepoint = codepoint;
            font_cache[hash].font = curfont;
            return curfont;
        }
    }
    return drw->fonts;
}

unsigned int drw_fontset_getwidth_fast(Drw *drw, const char *text) {
    if (!drw || !drw->fonts || !text) {
        return 0;
    }

    unsigned int width = 0;
    long utf8codepoint;
    int utf8err, utf8charlen;
    Fnt *curfont;

    while (*text) {
        utf8charlen = utf8decode(text, &utf8codepoint, &utf8err);
        if (utf8err) {
            text += utf8charlen;
            continue;
        }

        curfont = find_font_for_char(drw, utf8codepoint);
        unsigned int char_width = 0;
        drw_font_getexts(curfont, text, utf8charlen, &char_width, NULL);
        width += char_width;
        text += utf8charlen;
    }

    return width;
}

unsigned int drw_fontset_getwidth(Drw *drw, const char *text) {
    if (!drw || !drw->fonts || !text)
        return 0;
    return drw_fontset_getwidth_fast(drw, text);
}

unsigned int
drw_fontset_getwidth_clamp(Drw *drw, const char *text, unsigned int n) {
    unsigned int tmp = 0;
    if (drw && drw->fonts && text && n)
        tmp = drw_text(drw, 0, 0, 0, 0, 0, text, n);
    return MIN(n, tmp);
}

void inline drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h) {
    XGlyphInfo ext;

    if (!font || !text)
        return;

    XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
    if (w)
        *w = ext.xOff;
    if (h)
        *h = font->h;
}

Cur *drw_cur_create(Drw *drw, int shape) {
    Cur *cur;

    if (!drw || !(cur = ecalloc(1, sizeof(Cur))))
        return NULL;

    cur->cursor = XCreateFontCursor(drw->dpy, shape);

    return cur;
}

void drw_cur_free(Drw *drw, Cur *cursor) {
    if (!cursor)
        return;

    XFreeCursor(drw->dpy, cursor->cursor);
    free(cursor);
}
