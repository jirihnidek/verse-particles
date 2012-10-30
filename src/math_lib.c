/*
 * $Id$
 *
 * ***** BEGIN BSD LICENSE BLOCK *****
 *
 * Copyright (c) 2009-2011, Jiri Hnidek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ***** END BSD LICENSE BLOCK *****
 *
 * Authors: Jiri Hnidek <jiri.hnidek@tul.cz>
 *
 */

#include <math.h>
#include <verse.h>

#include "math_lib.h"

/**
 * \brief This function compute spherical coordinates from cartesian coordinates
 */
void cartesion_to_spherical(real32 cart[3], real32 spher[3])
{
	spher[0] = sqrt(cart[0]*cart[0] + cart[1]*cart[1] + cart[2]*cart[2]);
	spher[1] = acos(cart[2]/spher[0]);
	spher[2] = atan2(cart[1], cart[0]);
}

/**
 * \brief This function compute cartesian coordinates from spherical coordinates
 */
void spherical_to_cartesian(real32 spher[3], real32 cart[3])
{
	cart[0] = spher[0] * sin(spher[1]) * cos(spher[2]);
	cart[1] = spher[0] * sin(spher[1]) * sin(spher[2]);
	cart[2] = spher[0] * cos(spher[1]);
}

/*
 * Convert HSV values to RGB values
 *
 * All values are in the range <0.0, 1.0>
 */
int hsv2rgb(struct HSV_Color *hsv, struct RGB_Color *rgb)
{
	float S, H, V, F, M, N, K;
	int I;

	H = hsv->h; /* Hue */
	S = hsv->s; /* Saturation */
	V = hsv->v; /* value or brightness */

	if (S == 0.0) {
		/* Achromatic case, set level of grey */
		rgb->r = V;
		rgb->g = V;
		rgb->b = V;
	} else {
		/* Determine levels of primary colors.*/
		if (H >= 1.0) {
			H = 0.0;
		} else {
			H = H * 6;
		}

		I = (int) H;	/* should be in the range 0..5 */
		F = H - I;		/* fractional part */

		M = V * (1 - S);
		N = V * (1 - S * F);
		K = V * (1 - S * (1 - F));

		if (I == 0) {
			rgb->r = V;
			rgb->g = K;
			rgb->b = M;
		}
		if (I == 1) {
			rgb->r = N;
			rgb->g = V;
			rgb->b = M;
		}
		if (I == 2) {
			rgb->r = M;
			rgb->g = V;
			rgb->b = K;
		}
		if (I == 3) {
			rgb->r = M;
			rgb->g = N;
			rgb->b = V;
		}
		if (I == 4) {
			rgb->r = K;
			rgb->g = M;
			rgb->b = V;
		}
		if (I == 5) {
			rgb->r = V;
			rgb->g = M;
			rgb->b = N;
		}
	}

	return 0;
}
