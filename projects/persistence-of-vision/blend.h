/**
 * @file blend.h
 * @brief Porter-Duff Alpha compositing for POV multi-layer rendering
 *
 * Implements 13 standard Porter-Duff compositing operators using Q8
 * fixed-point arithmetic (0-1 mapped to 0-256, normalized via >> 8).
 *
 * Operators: Clear, Src, Dst, SrcOver, DstOver, SrcIn, DstIn,
 *            SrcOut, DstOut, SrcAtop, DstAtop, Xor, Plus
 *
 * @note All compositing functions operate on single pixels and are
 *       declared static inline for zero call overhead on Cortex-M7.
 */

#ifndef POV_BLEND_H
#define POV_BLEND_H

#include <linux/types.h>

/**
 * @brief RGBA pixel (4 bytes, matching GPU/GPU texture format layout)
 */
struct rgba_pixel {
	u8 blue;        /* Blue channel  (0-255) */
	u8 green;       /* Green channel (0-255) */
	u8 red;         /* Red channel   (0-255) */
	u8 alpha;       /* Alpha channel (0-255, 0=transparent, 255=opaque) */
};

/**
 * @brief Porter-Duff compositing operators
 *
 * S = Source (top layer), D = Destination (accumulated below)
 * All formulas use Q8 fixed point: alpha ∈ [0, 256] where 256 = 1.0
 */
enum pov_porter_duff_mode {
	POV_PD_CLEAR     = 0,    /* Co = 0                                    */
	POV_PD_SRC       = 1,    /* Co = Cs                                   */
	POV_PD_DST       = 2,    /* Co = Cd                                   */
	POV_PD_SRC_OVER  = 3,    /* Co = Cs + Cd×(1-As)    — default alpha    */
	POV_PD_DST_OVER  = 4,    /* Co = Cd + Cs×(1-Ad)                       */
	POV_PD_SRC_IN    = 5,    /* Co = Cs×Ad                                */
	POV_PD_DST_IN    = 6,    /* Co = Cd×As                                */
	POV_PD_SRC_OUT   = 7,    /* Co = Cs×(1-Ad)                            */
	POV_PD_DST_OUT   = 8,    /* Co = Cd×(1-As)                            */
	POV_PD_SRC_ATOP  = 9,    /* Co = Cs×Ad + Cd×(1-As)                    */
	POV_PD_DST_ATOP  = 10,   /* Co = Cd×As + Cs×(1-Ad)                    */
	POV_PD_XOR       = 11,   /* Co = Cs×(1-Ad) + Cd×(1-As)               */
	POV_PD_PLUS      = 12,   /* Co = min(Cs+Cd, 255) — additive blending  */
	POV_PD_MODE_COUNT
};

/* ── Single-Pixel Porter-Duff Compositing (Q8 Fixed-Point) ── */

/**
 * @brief Composite one RGBA pixel onto a destination using Q8 Porter-Duff
 *
 * @param dst   Destination pixel (bottom, modified in-place)
 * @param src   Source pixel (top layer)
 * @param mode  Porter-Duff operator
 *
 * All channels are 8-bit (0-255).  Alpha is treated as Q8 (0-256 range
 * for compositing math, with 256 = 1.0).  The destination alpha is
 * updated according to the compositing rules.
 */
static inline void porter_duff_composite_pixel(struct rgba_pixel *dst, struct rgba_pixel *src,
	enum pov_porter_duff_mode mode)
{
	u16 inv_sa;                     /* 256 - src->alpha */
	u16 inv_da;                     /* 256 - dst->alpha */
	u16 r, g, b, a;                 /* Result channels (wide to avoid overflow) */

	/* For non-zero values: alpha in [1,255] maps to [1,256] on Q8 scale.
	 * We use 256-as-1.0 convention: src->alpha represents As ∈ [0,1] as value/256.
	 * Since hardware alpha is 0-255, we compensate: As = src->alpha/255, but for
	 * fixed-point we approximate with (src->alpha+1) when sa≠0 — or simply use
	 * the standard GPU convention where alpha in [0,255] is used directly
	 * with 256 as 1.0 divisor (commonly used in GPU blending).  The error
	 * from treating 255 as 255/256 instead of 1.0 is <0.4%, acceptable. */
	inv_sa = 256 - src->alpha;
	inv_da = 256 - dst->alpha;

	switch (mode) {
	case POV_PD_CLEAR:
		dst->red   = 0;
		dst->green = 0;
		dst->blue  = 0;
		dst->alpha = 0;
		return;

	case POV_PD_SRC:
		*dst = *src;
		return;

	case POV_PD_DST:
		/* No change */
		return;

	case POV_PD_SRC_OVER:
		/* Co_rgb = Cs + Cd*(1-As), Co_a = As + Ad*(1-As) */
		r = (u16)src->red   + (((u16)dst->red   * inv_sa) >> 8);
		g = (u16)src->green + (((u16)dst->green * inv_sa) >> 8);
		b = (u16)src->blue  + (((u16)dst->blue  * inv_sa) >> 8);
		a = src->alpha + ((dst->alpha * inv_sa) >> 8);
		break;

	case POV_PD_DST_OVER:
		/* Co_rgb = Cd + Cs*(1-Ad), Co_a = Ad + As*(1-Ad) */
		r = (u16)dst->red   + (((u16)src->red   * inv_da) >> 8);
		g = (u16)dst->green + (((u16)src->green * inv_da) >> 8);
		b = (u16)dst->blue  + (((u16)src->blue  * inv_da) >> 8);
		a = dst->alpha + ((src->alpha * inv_da) >> 8);
		break;

	case POV_PD_SRC_IN:
		/* Co_rgb = Cs*Ad, Co_a = As*Ad */
		r = ((u16)src->red   * dst->alpha) >> 8;
		g = ((u16)src->green * dst->alpha) >> 8;
		b = ((u16)src->blue  * dst->alpha) >> 8;
		a = (src->alpha * dst->alpha) >> 8;
		break;

	case POV_PD_DST_IN:
		/* Co_rgb = Cd*As, Co_a = Ad*As */
		r = ((u16)dst->red   * src->alpha) >> 8;
		g = ((u16)dst->green * src->alpha) >> 8;
		b = ((u16)dst->blue  * src->alpha) >> 8;
		a = (dst->alpha * src->alpha) >> 8;
		break;

	case POV_PD_SRC_OUT:
		/* Co_rgb = Cs*(1-Ad), Co_a = As*(1-Ad) */
		r = ((u16)src->red   * inv_da) >> 8;
		g = ((u16)src->green * inv_da) >> 8;
		b = ((u16)src->blue  * inv_da) >> 8;
		a = (src->alpha * inv_da) >> 8;
		break;

	case POV_PD_DST_OUT:
		/* Co_rgb = Cd*(1-As), Co_a = Ad*(1-As) */
		r = ((u16)dst->red   * inv_sa) >> 8;
		g = ((u16)dst->green * inv_sa) >> 8;
		b = ((u16)dst->blue  * inv_sa) >> 8;
		a = (dst->alpha * inv_sa) >> 8;
		break;

	case POV_PD_SRC_ATOP:
		/* Co_rgb = Cs*Ad + Cd*(1-As), Co_a = Ad */
		r = (((u16)src->red   * dst->alpha) >> 8) + (((u16)dst->red   * inv_sa) >> 8);
		g = (((u16)src->green * dst->alpha) >> 8) + (((u16)dst->green * inv_sa) >> 8);
		b = (((u16)src->blue  * dst->alpha) >> 8) + (((u16)dst->blue  * inv_sa) >> 8);
		a = dst->alpha;
		break;

	case POV_PD_DST_ATOP:
		/* Co_rgb = Cd*As + Cs*(1-Ad), Co_a = As */
		r = (((u16)dst->red   * src->alpha) >> 8) + (((u16)src->red   * inv_da) >> 8);
		g = (((u16)dst->green * src->alpha) >> 8) + (((u16)src->green * inv_da) >> 8);
		b = (((u16)dst->blue  * src->alpha) >> 8) + (((u16)src->blue  * inv_da) >> 8);
		a = src->alpha;
		break;

	case POV_PD_XOR:
		/* Co_rgb = Cs*(1-Ad) + Cd*(1-As), Co_a = As+Ad - 2*As*Ad */
		r = (((u16)src->red   * inv_da) >> 8) + (((u16)dst->red   * inv_sa) >> 8);
		g = (((u16)src->green * inv_da) >> 8) + (((u16)dst->green * inv_sa) >> 8);
		b = (((u16)src->blue  * inv_da) >> 8) + (((u16)dst->blue  * inv_sa) >> 8);
		a = src->alpha + dst->alpha - ((src->alpha * dst->alpha) >> 7);  /* 2*As*Ad = (As*Ad)/256 * 2 */
		break;

	case POV_PD_PLUS:
		/* Co_rgb = min(Cs+Cd, 255), Co_a = min(As+Ad, 255) — additive */
		r = (u16)src->red   + (u16)dst->red;
		g = (u16)src->green + (u16)dst->green;
		b = (u16)src->blue  + (u16)dst->blue;
		a = src->alpha + dst->alpha;
		if (r > 255) {
			r = 255;
		}
		if (g > 255) {
			g = 255;
		}
		if (b > 255) {
			b = 255;
		}
		if (a > 255) {
			a = 255;
		}
		dst->red   = (u8)r;
		dst->green = (u8)g;
		dst->blue  = (u8)b;
		dst->alpha = (u8)a;
		return;

	default:
		/* Unknown mode: fall through to SRC_OVER */
		r = (u16)src->red   + (((u16)dst->red   * inv_sa) >> 8);
		g = (u16)src->green + (((u16)dst->green * inv_sa) >> 8);
		b = (u16)src->blue  + (((u16)dst->blue  * inv_sa) >> 8);
		a = src->alpha + ((dst->alpha * inv_sa) >> 8);
		break;
	}

	/* Clamp and store (channels may be up to 510 before clamp from two 8-bit terms) */
	dst->red   = (u8)(r > 255 ? 255 : r);
	dst->green = (u8)(g > 255 ? 255 : g);
	dst->blue  = (u8)(b > 255 ? 255 : b);
	dst->alpha = (u8)(a > 255 ? 255 : a);
}

/**
 * @brief Apply global alpha to a source pixel (modifies in-place)
 *
 * Effective alpha = pixel.alpha * global_alpha / 255
 *
 * @param pixel         Source pixel to modify
 * @param global_alpha  Global opacity (0-255, 255=fully opaque)
 */
static inline void porter_duff_apply_global_alpha(
	struct rgba_pixel *pixel, u8 global_alpha)
{
	if (global_alpha >= 255) {
		return;    /* No scaling needed */
	}

	pixel->alpha = (u8)(((u16)pixel->alpha * (u16)global_alpha) >> 8);
}

int pov_compose_layers(const char *path);

#endif /* POV_BLEND_H */
