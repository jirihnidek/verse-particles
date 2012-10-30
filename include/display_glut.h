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

#ifndef DISPLAY_GLUT_H_
#define DISPLAY_GLUT_H_

#include <verse.h>

#include "client.h"

typedef enum VisualType {
	VISUAL_SIMPLE		= 1,
	VISUAL_LINE		= 2,
	VISUAL_DOT		= 3,
	VISUAL_DOT_LINE	= 4
} VisualType;

static const uint8 red_col[3] = {255, 0, 0};
static const uint8 white_col[3] = {255, 255, 255};
static const uint8 gray_col[3] = {155, 155, 155};
static const uint8 light_red_col[3] = {200, 0, 0};
static const uint8 light_red_col_a[4] = {200, 0, 0, 150};
static const uint8 orange_col[3] = {255, 127, 0};
static const uint8 orange_col_a[4] = {255, 127, 0, 150};
static const uint8 yellow_col[3] = {255, 255, 0};
static const uint8 yellow_col_a[4] = {255, 255, 0, 150};
static const uint8 green_col[3] = {0, 200, 0};
static const uint8 green_col_a[4] = {0, 200, 0, 150};

/**
 * Information about surface
 */
typedef struct Material {
	real32	diffuse[4];
	real32	specular[4];
	real32	ambient[4];
} Material;

/**
 * Information about light
 */
typedef struct Light {
	real32	diffuse[4];
	real32	specular[4];
	real32	ambient[4];
	real32	pos[4];
} Light;

/**
 * Information about camera
 */
typedef struct Camera {
	real32	field_of_view;
	real32	near_clipping_plane;
	real32	far_clipping_plane;
	real32	pos[3];
	real32	target[3];
	real32	up[3];
} Camera;

/**
 * Information about window
 */
typedef struct Window {
	uint16	fullscreen;
	uint16	left;
	uint16	top;
	uint16	width;
	uint16	height;
} Window;

/**
 * Information about mouse cursor
 */
typedef struct Mouse {
	uint8	state;
	uint16	pos[2];
} Mouse;

/**
 * All information required to display basic scene
 */
typedef struct ParticleDisplay {
	struct Window		window;
	struct Mouse		mouse;
	struct Camera		camera;
	struct Material		material;
	struct Light		light;
	enum VisualType		visual_type;
} ParticleDisplay;

struct Client_CTX;

void particle_display_loop(struct Client_CTX *_ctx, int argc, char *argv[]);
struct ParticleDisplay *create_particle_display(void);

#endif /* DISPLAY_GLUT_H_ */
