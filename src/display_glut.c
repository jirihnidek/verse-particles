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

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <sys/time.h>
#include <stdio.h>
#include <math.h>

#include <verse.h>

#include "list.h"
#include "client.h"
#include "display_glut.h"
#include "math_lib.h"

static struct Client_CTX *ctx = NULL;

/**
 * \brief Initialize OpenGL context
 */
static void gl_init(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);	/* TODO: add to ctx */
	glClearDepth(1.0f);	/* TODO: add to ctx */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPointSize(2.0);	/* TODO: add to ctx */
	glLineWidth(1.0);	/* TODO: add to ctx */
	glEnable(GL_POINT_SMOOTH);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ctx->display->light.ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, ctx->display->light.diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, ctx->display->light.specular);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ctx->display->material.ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ctx->display->material.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ctx->display->material.specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0);	/* TODO: add to ctx */
}

static void display_string_2d(char *string, int x, int y, const uint8 *col)
{
	glColor3ubv(col);
	glRasterPos2f(x, y);
	while (*string) {
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *string++);
	}
}

/**
 * \brief This function display sender
 */
static void display_sender(struct Particle_Sender *sender)
{
	float pos[3];

	pos[0] = sender->pos[0];
	pos[1] = sender->pos[1];
	pos[2] = sender->pos[2];

	/* Plane emiting particles */
	glBegin(GL_LINE_LOOP);
	glColor3ubv(white_col);
	glVertex3f(pos[0]+1, pos[1]+4, pos[2]+2-1);
	glVertex3f(pos[0]+1, pos[1]+4, pos[2]+2+1);
	glVertex3f(pos[0]-1, pos[1]+4, pos[2]+2+1);
	glVertex3f(pos[0]-1, pos[1]+4, pos[2]+2-1);
	glEnd();

	/* TODO: dislay frame of sender and number of particle */

	/* Fake shadow of plane emiting particles */
	glLineWidth(2.0);
	glBegin(GL_LINES);
	glColor3ub(65, 65, 65);
	glVertex3f(pos[0]+1, pos[1]+4, pos[2]);
	glVertex3f(pos[0]-1, pos[1]+4, pos[2]);
	glEnd();
	glLineWidth(1.0);

	/* Collision plane */
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glBegin(GL_QUADS);
	glVertex3f(pos[0]+8, pos[1]+5,  pos[2]-0.00001);
	glVertex3f(pos[0]-8, pos[1]+5,  pos[2]-0.00001);
	glVertex3f(pos[0]-8, pos[1]-11, pos[2]-0.00001);
	glVertex3f(pos[0]+8, pos[1]-11, pos[2]-0.00001);
	glEnd();
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
}

#define MAX_STR_LEN	100

/**
 * \brief This function draw text information about scene
 */
static void display_text_info(void)
{
	static int screen_counter = 0;
	static struct timeval tv, last;
	static float sfps = 0;
	float fps;
	/* int16 frame, received_frame */
	int tmp;
	char str_frame[MAX_STR_LEN];
	short x_pos = 0, y_pos = 0;

	x_pos = 10;
	y_pos = 10;

	/* Draw help */
	y_pos += 15;
	if(ctx->timer->tot_frame < ctx->pd->frame_count) {
		tmp = snprintf(str_frame, MAX_STR_LEN-1,
				"Q: Quit, ESC: Force Quit, F: Fullscreen, C: Cycle visual style, LMB: Pan, RMB: Move, MMB: Zoom");
	} else {
		tmp = snprintf(str_frame, MAX_STR_LEN-1,
				"Q: Quit, ESC: Force Quit, F: Fullscreen, C: Cycle visual style, R: Reset, LMB: Pan, RMB: Move, MMB: Zoom");
	}
	str_frame[tmp] = '\0';
	display_string_2d(str_frame, x_pos, y_pos, white_col);

#if 0
	/* Draw frame number */
	y_pos += 15;
	pthread_mutex_lock(&ctx->timer->mutex);
	if(ctx->verse.particle_scene_node != NULL) {
		received_frame = ctx->verse.particle_scene_node->received_frame;
	} else {
		received_frame = -1;
	}
	frame = (ctx->timer->tot_frame < ctx->pd->frame_count) ? ctx->timer->tot_frame : ctx->pd->frame_count-1;
	tmp = snprintf(str_frame, MAX_STR_LEN-1, "Frame: %d (%d)", frame, received_frame);
	pthread_mutex_unlock(&ctx->timer->mutex);
	str_frame[tmp] = '\0';
	display_string_2d(str_frame, x_pos, y_pos, white_col);
#endif

	/* Compute value of FPS */
	if(screen_counter==0) {
		gettimeofday(&last, NULL);
		fps = 0.0;
	} else {
		unsigned long int delay;
		gettimeofday(&tv, NULL);
		delay = (tv.tv_sec - last.tv_sec)*1000000 + (tv.tv_usec - last.tv_usec);
		fps = (float)1000000.0/(float)delay;
		if(sfps==0) {
			sfps = fps;
		} else {
			sfps = (1 - 0.1)*sfps + (0.1)*fps;
		}
		last.tv_sec = tv.tv_sec;
		last.tv_usec = tv.tv_usec;
	}

	/* Draw smoothed FPS */
	y_pos += 15;
	tmp = snprintf(str_frame, MAX_STR_LEN-1, "FPS: %5.1f", sfps);
	str_frame[tmp] = '\0';
	display_string_2d(str_frame, x_pos, y_pos, white_col);

	/* Draw visual style */
	y_pos += 15;
	switch(ctx->display->visual_type) {
	case VISUAL_SIMPLE:
		tmp = snprintf(str_frame, MAX_STR_LEN-1, "Visualization mode: SIMPLE");
		break;
	case VISUAL_DOT:
		tmp = snprintf(str_frame, MAX_STR_LEN-1, "Visualization mode: DOTS");
		break;
	case VISUAL_LINE:
		tmp = snprintf(str_frame, MAX_STR_LEN-1, "Visualization mode: LINES");
		break;
	case VISUAL_DOT_LINE:
		tmp = snprintf(str_frame, MAX_STR_LEN-1, "Visualization mode: LINES & DOTS");
		break;
	}
	str_frame[tmp] = '\0';
	display_string_2d(str_frame, x_pos, y_pos, white_col);

	screen_counter++;
}

/**
 * \brief This function display one particle
 */
static void display_particle(float *pos, float size, const uint8 *col, const uint8 shadow)
{
	float val;

	/* Display last received position of (lost/delayed) particle */
	glPointSize(size);
	glBegin(GL_POINTS);
	glColor3ubv(col);
	glVertex3fv(pos);

	/* Fake shadow of particle */
	if(shadow == 1) {
		val = (pos[2]<1.0) ? pos[2]/4.0 : 0.25;
		glColor3f(val, val, val);
		glVertex3f(pos[0], pos[1], 0.0);
	}

	glEnd();
	glPointSize(2.0);
}

/**
 * \brief This function try to display received particle
 */
static void display_rec_particle_simple(struct ReceivedParticle *rec_particle,
		int current_frame)
{
	if(rec_particle->current_received_state != NULL) {
		switch(rec_particle->current_received_state->state) {
		case RECEIVED_STATE_INTIME:
		case RECEIVED_STATE_AHEAD:
			display_particle(rec_particle->current_received_state->ref_particle_state->pos,
					2.0,
					yellow_col,
					1);
			break;
		case RECEIVED_STATE_DELAY:
			if(rec_particle->current_received_state->ref_particle_state->frame < rec_particle->ref_particle->die_frame-1) {
				display_particle(rec_particle->current_received_state->ref_particle_state->pos,
						4.0,
						red_col,
						1);
			}
			display_particle(rec_particle->ref_particle->states[current_frame].pos,
					2.0,
					white_col,
					1);

			break;
		default:
			break;
		}

	} else if(rec_particle->ref_particle->born_frame <= current_frame) {
		display_particle(rec_particle->ref_particle->states[rec_particle->ref_particle->born_frame].pos,
						4.0,
						red_col,
						1);
	}
}

/**
 * \brief This function display line with delay between currently received
 * particle position and expected particle position
 */
static void display_rec_particle_lines(struct ReceivedParticle *rec_particle,
		int current_frame)
{
	int frame;

	if(rec_particle->ref_particle->states[current_frame].state != PARTICLE_STATE_UNBORN &&
			rec_particle->last_received_state != NULL) {
		struct HSV_Color hsv;
		struct RGB_Color rgb;
		float dist, dx, dy, dz;
		hsv.s = 1.0;
		hsv.v = 1.0;

		glBegin(GL_LINE_STRIP);
		for(frame = rec_particle->last_received_state->ref_particle_state->frame;
				frame < current_frame;
				frame++)
		{
			dx = rec_particle->last_received_state->ref_particle_state->pos[0] - rec_particle->ref_particle->states[frame].pos[0];
			dy = rec_particle->last_received_state->ref_particle_state->pos[1] - rec_particle->ref_particle->states[frame].pos[1];
			dz = rec_particle->last_received_state->ref_particle_state->pos[2] - rec_particle->ref_particle->states[frame].pos[2];

			dist = sqrt(dx*dx + dy*dy + dz*dz);
			hsv.h = (dist<10) ? 0.1*dist : 1.0;
			hsv2rgb(&hsv, &rgb);

			glColor3f(rgb.r, rgb.g, rgb.b);
			glVertex3fv(rec_particle->ref_particle->states[frame].pos);
		}
		glEnd();
	}
}

/**
 * \brief This function display history of received particles
 */
static void display_rec_particle_dots(struct ReceivedParticle *rec_particle,
		int current_frame)
{
	int frame;

	if(rec_particle->ref_particle->states[current_frame].state != PARTICLE_STATE_UNBORN) {

		glColor3ubv(gray_col);
		glBegin(GL_LINE_STRIP);
		for(frame=rec_particle->ref_particle->born_frame; frame<current_frame; frame++) {
			glVertex3fv(rec_particle->received_states[frame].ref_particle_state->pos);
		}
		glEnd();

		for(frame=rec_particle->ref_particle->born_frame; frame<current_frame; frame++) {
			switch(rec_particle->received_states[frame].state) {
			case RECEIVED_STATE_UNRECEIVED:
				display_particle(rec_particle->received_states[frame].ref_particle_state->pos,
						2.0,
						red_col,
						0);
				break;
			case RECEIVED_STATE_DELAY:
				display_particle(rec_particle->received_states[frame].ref_particle_state->pos,
						2.0,
						orange_col,
						0);
				break;
			case RECEIVED_STATE_INTIME:
				display_particle(rec_particle->received_states[frame].ref_particle_state->pos,
						2.0,
						green_col,
						0);
				break;
			default:
				break;
			}
		}

	}
}

/**
 * \brief This function displays received particle system
 */
static void display_rec_particle_system(struct Particle_Sender *sender)
{
	int i, current_frame;

	pthread_mutex_lock(&sender->rec_pd->mutex);

	/* Get current frame */
	pthread_mutex_lock(&ctx->timer->mutex);
	current_frame = ctx->timer->frame;
	pthread_mutex_unlock(&ctx->timer->mutex);

	glTranslatef(sender->pos[0], sender->pos[1], sender->pos[2]);

	/* Display particle system only in situation, when animation was started */
	if(current_frame >= 0) {
		for(i=0; i<ctx->pd->particle_count; i++) {
			switch(ctx->display->visual_type) {
			case VISUAL_DOT:
				display_rec_particle_dots(&sender->rec_pd->received_particles[i], current_frame);
				display_rec_particle_simple(&sender->rec_pd->received_particles[i], current_frame);
				break;
			case VISUAL_LINE:
				display_rec_particle_lines(&sender->rec_pd->received_particles[i], current_frame);
				display_rec_particle_simple(&sender->rec_pd->received_particles[i], current_frame);
				break;
			case VISUAL_DOT_LINE:
				display_rec_particle_dots(&sender->rec_pd->received_particles[i], current_frame);
				display_rec_particle_lines(&sender->rec_pd->received_particles[i], current_frame);
				display_rec_particle_simple(&sender->rec_pd->received_particles[i], current_frame);
				break;
			case VISUAL_SIMPLE:
				display_rec_particle_simple(&sender->rec_pd->received_particles[i], current_frame);
				break;
			}
		}
	}

	glTranslatef(-sender->pos[0], -sender->pos[1], -sender->pos[2]);

	pthread_mutex_unlock(&sender->rec_pd->mutex);
}


/**
 * \brief This function displays 3d scene
 */
static void glut_on_display(void)
{
	struct Particle_Sender *sender;

	glViewport(0, 0, ctx->display->window.width, ctx->display->window.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, ctx->display->window.width, 0, ctx->display->window.height);
	glScalef(1, -1, 1);
	glTranslatef(0, -ctx->display->window.height, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* BEGIN: Drawing of 2D stuff */

	display_text_info();

	/* END: Drawing of 2D stuff */

	/* Set projection matrix for 3D stuff here */
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(ctx->display->camera.field_of_view,
			(double)ctx->display->window.width/(double)ctx->display->window.height,
			ctx->display->camera.near_clipping_plane,
			ctx->display->camera.far_clipping_plane);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	gluLookAt(ctx->display->camera.pos[0], ctx->display->camera.pos[1], ctx->display->camera.pos[2],
			ctx->display->camera.target[0], ctx->display->camera.target[1], ctx->display->camera.target[2],
			ctx->display->camera.up[0], ctx->display->camera.up[1], ctx->display->camera.up[2]);
	glPushMatrix();

	/* BEGIN: Drawing of 3d staff */

	sender = ctx->senders.first;
	while(sender != NULL) {
		/* Display emitter and collision plane of sender */
		display_sender(sender);
		/* Display received particles */
		display_rec_particle_system(sender);

		sender = sender->next;
	}

	/* END: Drawing of 3d staff */

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glFlush();
	glutSwapBuffers();
}

/**
 * \brief Display particles in regular periods
 */
static void glut_on_timer(int value) {
	glutPostRedisplay();
	glutTimerFunc(40, glut_on_timer, value);	/* TODO: timer */
}

/**
 * Callback function called on window resize
 */
static void glut_on_resize(int w, int h)
{
	ctx->display->window.width = w;
	ctx->display->window.height = h;
}

/**
 * \brief This function is called on mouse click
 */
static void glut_on_mouse_click(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON) {
		/* Start rotate of camera */
		if(state == GLUT_DOWN) {
			ctx->display->mouse.state = GLUT_LEFT_BUTTON;
			ctx->display->mouse.pos[0] = x;
			ctx->display->mouse.pos[1] = y;
		} else {
			ctx->display->mouse.state = -1;
		}
	} else if(button == GLUT_RIGHT_BUTTON) {
		/* Start move of camera */
		if(state == GLUT_DOWN) {
			ctx->display->mouse.state = GLUT_RIGHT_BUTTON;
			ctx->display->mouse.pos[0] = x;
			ctx->display->mouse.pos[1] = y;
		} else {
			ctx->display->mouse.state = -1;
		}
	} else if(button == GLUT_MIDDLE_BUTTON) {
		/* Start zoom in/out of camera */
		/* Start rotate of camera */
		if(state == GLUT_DOWN) {
			ctx->display->mouse.state = GLUT_MIDDLE_BUTTON;
			ctx->display->mouse.pos[0] = x;
			ctx->display->mouse.pos[1] = y;
		} else {
			ctx->display->mouse.state = -1;
		}
	}
}

/**
 * \brief This function is called on mouse move
 */
static void glut_on_mouse_drag(int x, int y)
{
	int32 dx, dy;

	if(ctx->display->mouse.state == GLUT_LEFT_BUTTON) {
		real32 rel_pos[3];
		real32 sphere[3];

		rel_pos[0] = ctx->display->camera.pos[0] - ctx->display->camera.target[0];
		rel_pos[1] = ctx->display->camera.pos[1] - ctx->display->camera.target[1];
		rel_pos[2] = ctx->display->camera.pos[2] - ctx->display->camera.target[2];

		cartesion_to_spherical(rel_pos, sphere);

		dx = x - ctx->display->mouse.pos[0];
		dy = y - ctx->display->mouse.pos[1];

		if(abs(dy) < 100) {
			sphere[1] -= (float)dy/100.0;
		}
		if(abs(dx) < 100) {
			sphere[2] -= (float)dx/100.0;
		}

		spherical_to_cartesian(sphere, rel_pos);

		ctx->display->camera.pos[0] = rel_pos[0] + ctx->display->camera.target[0];
		ctx->display->camera.pos[1] = rel_pos[1] + ctx->display->camera.target[1];
		ctx->display->camera.pos[2] = rel_pos[2] + ctx->display->camera.target[2];

		ctx->display->mouse.pos[0] = x;
		ctx->display->mouse.pos[1] = y;

	} else if(ctx->display->mouse.state == GLUT_RIGHT_BUTTON) {
		float fdx, fdy;

		fdx = ctx->display->camera.target[0] - ctx->display->camera.pos[0];
		fdy = ctx->display->camera.target[1] - ctx->display->camera.pos[1];

		dx = x - ctx->display->mouse.pos[0];
		dy = y - ctx->display->mouse.pos[1];

		if(abs(dx) < 100) {
			ctx->display->camera.target[0] -= 0.001*fdy*dx;
			ctx->display->camera.pos[0]    -= 0.001*fdy*dx;

			ctx->display->camera.target[1] -= 0.001*(-fdx)*dx;
			ctx->display->camera.pos[1]    -= 0.001*(-fdx)*dx;
		}

		ctx->display->mouse.pos[0] = x;
		ctx->display->mouse.pos[1] = y;

	} else if(ctx->display->mouse.state == GLUT_MIDDLE_BUTTON) {
		real32 rel_pos[3];
		real32 sphere[3];

		rel_pos[0] = ctx->display->camera.pos[0] - ctx->display->camera.target[0];
		rel_pos[1] = ctx->display->camera.pos[1] - ctx->display->camera.target[1];
		rel_pos[2] = ctx->display->camera.pos[2] - ctx->display->camera.target[2];

		cartesion_to_spherical(rel_pos, sphere);

		dx = x - ctx->display->mouse.pos[0];
		dy = y - ctx->display->mouse.pos[1];

		if(abs(dy) < 100) {
			sphere[0] += (float)dy/5.0;
		}

		spherical_to_cartesian(sphere, rel_pos);

		ctx->display->camera.pos[0] = rel_pos[0] + ctx->display->camera.target[0];
		ctx->display->camera.pos[1] = rel_pos[1] + ctx->display->camera.target[1];
		ctx->display->camera.pos[2] = rel_pos[2] + ctx->display->camera.target[2];

		ctx->display->mouse.pos[0] = x;
		ctx->display->mouse.pos[1] = y;
	}

	/* TODO: send avatar position and orientation to verse server */

	/* TODO: update sender node priorities */
}

/**
 * \brief This function handle keyboard
 */
void glut_on_keyboard(unsigned char key, int x, int y)
{
	/* Was key pressed inside window? */
	if((x > 0) && (x < ctx->display->window.width) &&
			(y > 0) && (y < ctx->display->window.height)) {
		key = (key > 'A' && key <= 'Z') ? key + 'a' - 'A' : key;

		switch (key) {
		case 27:
			exit(0);
			break;
		case 'q':
			vrs_send_connect_terminate(ctx->verse.session_id);
			break;
		case 'f':
			if(ctx->display->window.fullscreen == 0) {
				glutFullScreen();
				ctx->display->window.fullscreen = 1;
			} else {
				ctx->display->window.fullscreen = 0;
				glutReshapeWindow(ctx->display->window.width, ctx->display->window.height);
				glutPositionWindow(ctx->display->window.left, ctx->display->window.top);
			}
			break;
		case 'c':
			/* Cycle through visual types */
			if(ctx->display->visual_type == VISUAL_SIMPLE)
				ctx->display->visual_type = VISUAL_LINE;
			else if(ctx->display->visual_type == VISUAL_LINE)
				ctx->display->visual_type = VISUAL_DOT;
			else if(ctx->display->visual_type == VISUAL_DOT)
				ctx->display->visual_type = VISUAL_DOT_LINE;
			else if(ctx->display->visual_type == VISUAL_DOT_LINE)
				ctx->display->visual_type = VISUAL_SIMPLE;
			break;
		default:
			break;
		}
	}
}

/**
 * \brief This function set up glut callback functions and basic settings
 */
static void glut_init(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowSize(ctx->display->window.width, ctx->display->window.height);
	glutInitWindowPosition(ctx->display->window.left, ctx->display->window.top);
	glutCreateWindow("Particles");
	glutDisplayFunc(glut_on_display);
	glutReshapeFunc(glut_on_resize);
	glutKeyboardFunc(glut_on_keyboard);
	glutMouseFunc(glut_on_mouse_click);
	glutMotionFunc(glut_on_mouse_drag);
	glutTimerFunc(40, glut_on_timer, 0);
	gl_init();
}

/**
 * \brief This function start Glut never ending loop
 */
void particle_display_loop(struct Client_CTX *_ctx, int argc, char *argv[])
{
	ctx = _ctx;

	glut_init(argc, argv);

	if(ctx != NULL) {
		glutMainLoop();
	}
}

/**
 * \brief This function create new structure holding informations about display
 */
struct ParticleDisplay *create_particle_display(void)
{
	struct ParticleDisplay *disp = NULL;

	disp = (struct ParticleDisplay*)malloc(sizeof(struct ParticleDisplay));

	disp->window.fullscreen = 0;
	disp->window.left = 100;
	disp->window.top = 100;
	disp->window.width = 800;
	disp->window.height = 600;

	disp->mouse.state = -1;
	disp->mouse.pos[0] = disp->mouse.pos[1] = 0;

	disp->camera.field_of_view = 45.0;
	disp->camera.near_clipping_plane = 0.1;
	disp->camera.far_clipping_plane = 200.0;

	disp->camera.pos[0] = 15.0;
	disp->camera.pos[1] = -15.0;
	disp->camera.pos[2] = 10.0;

	disp->camera.target[0] = 0.0;
	disp->camera.target[1] = -3.0;
	disp->camera.target[2] = 0.0;

	disp->camera.up[0] = 0.0;
	disp->camera.up[1] = 0.0;
	disp->camera.up[2] = 1.0;

	disp->light.ambient[0] = disp->light.ambient[1] = disp->light.ambient[2] = 0.7;
	disp->light.ambient[3] = 1.0;
	disp->light.diffuse[0] = disp->light.diffuse[1] = disp->light.diffuse[2] = 0.9;
	disp->light.diffuse[3] = 1.0;
	disp->light.specular[0] = disp->light.specular[1] = disp->light.specular[2] = 0.5;
	disp->light.specular[3] = 1.0;
	disp->light.pos[0] = 0.0;
	disp->light.pos[1] = 0.0;
	disp->light.pos[2] = 5.0;
	disp->light.pos[3] = 0.0;

	disp->material.ambient[0] = disp->material.ambient[1] = disp->material.ambient[2] = 0.2;
	disp->material.ambient[3] = 1.0;
	disp->material.diffuse[0] = disp->material.diffuse[1] = disp->material.diffuse[2] = 0.3;
	disp->material.diffuse[3] = 1.0;
	disp->material.specular[0] = disp->material.specular[1] = disp->material.specular[2] = 1.0;
	disp->material.specular[3] = 0.5;

	disp->visual_type = VISUAL_SIMPLE;

	return disp;
}
