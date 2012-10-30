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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "particle_data.h"
#include "client.h"

/**
 * \brief This function free reference particle data
 */
void free_ref_particle_data(struct RefParticleData *pd)
{
	int i;

	for(i=0; i<pd->particle_count; i++) {
		free(pd->particles[i].states);
		pd->particles[i].states = NULL;
	}

	free(pd->particles);
	pd->particles = NULL;
}

void print_ref_particle_data(struct RefParticleData *pd)
{
	int id, frame;

	for(frame=0; frame < pd->frame_count; frame++) {
		printf("Frame: %d\n", frame);
		for(id=0; id < pd->particle_count; id++) {
			printf("Id: %d, ", id);
			switch(pd->particles[id].states[frame].state) {
			case PARTICLE_STATE_RESERVED:
				printf("State: RESERVED, ");
				break;
			case PARTICLE_STATE_UNBORN:
				printf("State: UNBORN  , ");
				break;
			case PARTICLE_STATE_ACTIVE:
				printf("State: ACTIVE  , ");
				break;
			case PARTICLE_STATE_DEAD:
				printf("State: DEAD    , ");
				break;
			}
			printf("Pos: %6.3f %6.3f %6.3f\n",
					pd->particles[id].states[frame].pos[0],
					pd->particles[id].states[frame].pos[1],
					pd->particles[id].states[frame].pos[2]);
		}
	}
}

static void post_process_ref_particle_data(struct RefParticleData *pd)
{
	int id, frame, particle_is_born, particle_is_dead;

	/* Set up states of particles */
	for(id=0; id < pd->particle_count; id++) {
		particle_is_born = 0;
		particle_is_dead = 0;

		for(frame=0; frame<pd->frame_count; frame++) {

			/* Default state */
			pd->particles[id].states[frame].state = PARTICLE_STATE_RESERVED;

			/* Was particle born yet? */
			if(particle_is_born == 0 &&
					/* Is position at current frame the same as position at first frame? */
					pd->particles[id].states[frame].pos[0] == pd->particles[id].states[0].pos[0] &&
					pd->particles[id].states[frame].pos[1] == pd->particles[id].states[0].pos[1] &&
					pd->particles[id].states[frame].pos[2] == pd->particles[id].states[0].pos[2])
			{
				pd->particles[id].states[frame].state = PARTICLE_STATE_UNBORN;
			} else {

				/* Was particle born at this frame? */
				if(particle_is_born == 0) {
					pd->particles[id].states[frame].state = PARTICLE_STATE_ACTIVE;
					pd->particles[id].born_frame = frame;
					particle_is_born = 1;
				}
			}

			if(particle_is_born == 1 && particle_is_dead == 0) {
				pd->particles[id].states[frame].state = PARTICLE_STATE_ACTIVE;
			}

			/* Is particle dead? */
			if(particle_is_born == 1 && particle_is_dead == 0 &&
					/* Is position at current frame the same as position at previous frame? */
					pd->particles[id].states[frame].pos[0] == pd->particles[id].states[frame-1].pos[0] &&
					pd->particles[id].states[frame].pos[1] == pd->particles[id].states[frame-1].pos[1] &&
					pd->particles[id].states[frame].pos[2] == pd->particles[id].states[frame-1].pos[2])
			{
				if(particle_is_dead==0) {
					pd->particles[id].states[frame].state = PARTICLE_STATE_DEAD;
					pd->particles[id].die_frame = frame;
					particle_is_dead = 1;
				}
			} else if(particle_is_born == 1 && particle_is_dead == 1) {
				pd->particles[id].states[frame].state = PARTICLE_STATE_DEAD;
			}

		}
	}
}

/**
 * \brief This function loads reference particle data
 */
struct RefParticleData *read_ref_particle_data(char *dir_name)
{
	struct RefParticleData *pd;
	DIR *dir;
	struct dirent *dir_cont;

	int fd, id, frame_count, dir_name_len, file_name_len, file_path_len=0;
	float pos[3];
	float vel[3];
	char header[9];
	int particle_count, max_particle_count=0, type, data_type;
	int buf_pos = 0;
	char *file_path;

	/* Try to open directory with reference particle system */
	if( (dir = opendir(dir_name)) == NULL) {
		printf("Error: can't open directory: %s\n", dir_name);
		return NULL;
	}

	pd = (struct RefParticleData*)malloc(sizeof(struct RefParticleData));

	/* Get length of the directory name */
	dir_name_len = strlen(dir_name);

	/* Get number of particle count and particle states (number of frames) */
	frame_count = 0;
	while( (dir_cont = readdir(dir)) != NULL ) {

		/* Skip current directory and parent directory */
		if(strcmp(dir_cont->d_name, ".") == 0 ||
				strcmp(dir_cont->d_name, "..") == 0) continue;

		/* Get length of the file name */
		file_name_len = strlen(dir_cont->d_name);
		file_path_len = dir_name_len + 1 + file_name_len;
		file_path = malloc(sizeof(char)*file_path_len + 1);
		strcpy(file_path, dir_name);
		strcpy(&file_path[dir_name_len], "/");
		strcpy(&file_path[dir_name_len+1], dir_cont->d_name);
		file_path[file_path_len] = '\0';

		/* Try to open file */
		fd = open(file_path, O_RDONLY);
		if(fd == -1) {
			printf("Error: can't read file: %s\n", file_path);
		} else {
			buf_pos = 0;
			memset(header, 0, 9);
			buf_pos += read(fd, header, 8);
			if( strncmp(header, "BPHYSICS", 8) == 0) {
				buf_pos += read(fd, &type, sizeof(int));
				buf_pos += read(fd, &particle_count, sizeof(int));
				buf_pos += read(fd, &data_type, sizeof(int));

				if(particle_count>max_particle_count) {
					max_particle_count = particle_count;
				}

				frame_count++;
			} else {
				printf("Warning: file %s isn't particle data file, skipping.\n", file_path);
			}
			close(fd);
		}
		free(file_path);
	}

	pd->particle_count = max_particle_count;
	pd->frame_count = frame_count;

	printf("Debug: number of particles: %d, number of frames: %d\n", pd->particle_count, pd->frame_count);

	/* Allocate memory for reference particle system */
	pd->particles = (struct RefParticle*)calloc(pd->particle_count, sizeof(struct RefParticle));
	for(id=0; id < pd->particle_count; id++) {
		pd->particles[id].states = (struct RefParticleState*)calloc(pd->frame_count, sizeof(struct RefParticleState));
	}

	/* Try to open directory with reference particle system once again */
	if( (dir = opendir(dir_name)) == NULL) {
		printf("Error: can't open directory: %s\n", dir_name);
		free_ref_particle_data(pd);
		free(pd);
		return NULL;
	}

	/* Read content of directory once again and load data files */
	while( (dir_cont = readdir(dir)) != NULL ) {

		if(strcmp(dir_cont->d_name, ".") == 0 ||
				strcmp(dir_cont->d_name, "..") == 0) continue;

		file_name_len = strlen(dir_cont->d_name);
		file_path_len = dir_name_len + 1 + file_name_len;
		file_path = malloc(sizeof(char)*file_path_len + 1);
		strcpy(file_path, dir_name);
		strcpy(&file_path[dir_name_len], "/");
		strcpy(&file_path[dir_name_len+1], dir_cont->d_name);
		file_path[file_path_len] = '\0';

		fd = open(file_path, O_RDONLY);

		if(fd == -1) {
			printf("Error: can't read file: %s\n", file_path);
		} else {
			buf_pos = 0;
			memset(header, 0, 9);
			buf_pos += read(fd, header, 8);
			if( strncmp(header, "BPHYSICS", 8) == 0) {
				int frame;

				buf_pos += read(fd, &type, sizeof(int));
				buf_pos += read(fd, &particle_count, sizeof(int));
				buf_pos += read(fd, &data_type, sizeof(int));

				sscanf(&dir_cont->d_name[file_name_len-15], "%d", &frame);

				if( (frame > pd->frame_count) || (frame<1)) {
					printf("Error: bad frame number: %d\n", frame);
					continue;
				}

				/*
				printf("File: %s, particle_frame: %d, particle_count: %d\n",
						file_path, frame, particle_count);
				*/

				for(id=0; id < particle_count; id++) {
					pd->particles[id].id = id;

					pd->particles[id].states[frame-1].frame = frame-1;

					/* Read position */
					buf_pos += read(fd, &pos[0], sizeof(float));
					buf_pos += read(fd, &pos[1], sizeof(float));
					buf_pos += read(fd, &pos[2], sizeof(float));

					pd->particles[id].states[frame-1].pos[0] = pos[0];
					pd->particles[id].states[frame-1].pos[1] = pos[1];
					pd->particles[id].states[frame-1].pos[2] = pos[2];

					/* Read velocity */
					buf_pos += read(fd, &vel[0], sizeof(float));
					buf_pos += read(fd, &vel[1], sizeof(float));
					buf_pos += read(fd, &vel[2], sizeof(float));

					pd->particles[id].states[frame-1].vel[0] = vel[0];
					pd->particles[id].states[frame-1].vel[1] = vel[1];
					pd->particles[id].states[frame-1].vel[2] = vel[2];

					pd->particles[id].states[frame-1].state = PARTICLE_STATE_RESERVED;
				}
			} else {
				printf("Warning: file %s isn't particle data file, skipping.\n", file_path);
			}

			/* Close file */
			close(fd);
		}

		free(file_path);
		file_path = NULL;
	}

	/* Mark states of particles */
	post_process_ref_particle_data(pd);

	/* Debug print */
	/*print_ref_particle_data(pd);*/

	return pd;
}

/**
 * \brief This particle tries to find reference particle according received frame
 * and position
 */
struct RefParticleState *find_ref_particle_state(struct RefParticleData *pd,
		struct RefParticle *ref_particle,
		const int16 frame,
		const real32 pos[3])
{
	struct RefParticleState *ref_state = NULL;
	int i;

	if(ref_particle->states[frame].pos[0] == pos[0] &&
			ref_particle->states[frame].pos[1] == pos[1] &&
			ref_particle->states[frame].pos[2] == pos[2])
	{
		ref_state = &ref_particle->states[frame];
	} else {
		/* First, try to find delayed particle */
		for(i=frame; i>=0; i--) {
			if(ref_particle->states[i].pos[0] == pos[0] &&
					ref_particle->states[i].pos[1] == pos[1] &&
					ref_particle->states[i].pos[2] == pos[2])
			{
				ref_state = &ref_particle->states[i];
				break;
			}
		}

		/* Then try to find too fast particle */
		if(ref_state == NULL) {
			for(i=frame; i<pd->frame_count; i++) {
				if(ref_particle->states[i].pos[0] == pos[0] &&
						ref_particle->states[i].pos[1] == pos[1] &&
						ref_particle->states[i].pos[2] == pos[2])
				{
					ref_state = &ref_particle->states[i];
					break;
				}
			}
		}
	}

	return ref_state;
}

/**
 * \brief This function free received particle data
 */
void free_received_particle_data(struct ReceivedParticleData *rpd)
{
	int i;

	pthread_mutex_destroy(&rpd->mutex);

	if(rpd->received_particles != NULL) {
		for(i=0; i<rpd->ref_particle_data->particle_count; i++) {
			if(rpd->received_particles[i].received_states) {
				free(rpd->received_particles[i].received_states);
				rpd->received_particles[i].received_states = NULL;
			}
		}
		free(rpd->received_particles);
		rpd->received_particles = NULL;
	}
}

/**
 * \brief This function reset received particle data
 */
void reset_received_particle_data(struct ReceivedParticleData *rpd)
{
	int i, j;

	pthread_mutex_lock(&rpd->mutex);

	for(i=0; i < rpd->ref_particle_data->particle_count; i++) {
		/* Set up initial values */
		rpd->received_particles[i].first_received_state = NULL;
		rpd->received_particles[i].last_received_state = NULL;
		rpd->received_particles[i].current_received_state = NULL;

		for(j=0; j < rpd->ref_particle_data->frame_count; j++) {
			/* Set up initial values */
			rpd->received_particles[i].received_states[j].received_frame = 0;
			rpd->received_particles[i].received_states[j].delay = 0;
			rpd->received_particles[i].received_states[j].state = RECEIVED_STATE_UNRECEIVED;
		}
	}
	pthread_mutex_unlock(&rpd->mutex);
}

/**
 * \brief This function creates new structure for storing received particles positions
 */
struct ReceivedParticleData *create_received_particle_data(struct Client_CTX *ctx)
{
	struct RefParticleData *pd = ctx->pd;
	struct ReceivedParticleData *rpd = NULL;
	int i,j;

	rpd = (struct ReceivedParticleData*)malloc(sizeof(struct ReceivedParticleData));

	if(rpd != NULL) {
		pthread_mutex_init(&rpd->mutex, NULL);
		rpd->rec_frame = -1;
		rpd->ref_particle_data = pd;

		/* Create array of received particles */
		rpd->received_particles = (struct ReceivedParticle *)calloc(pd->particle_count, sizeof(struct ReceivedParticle));
		if(rpd->received_particles) {
			/* Initialize each particle */
			for(i=0; i<pd->particle_count; i++) {
				rpd->received_particles[i].first_received_state = NULL;
				rpd->received_particles[i].last_received_state = NULL;
				rpd->received_particles[i].current_received_state = NULL;
				rpd->received_particles[i].ref_particle = &pd->particles[i];
				/* Create array of received particle states */
				rpd->received_particles[i].received_states = (struct ReceivedParticleState*)calloc(pd->frame_count, sizeof(struct ReceivedParticleState));
				if(rpd->received_particles[i].received_states != NULL) {
					/* Initialize each state */
					for(j=0; j<pd->frame_count; j++) {
						/* Create reference */
						rpd->received_particles[i].received_states[j].ref_particle_state = &pd->particles[i].states[j];
						/* Set up initial values */
						rpd->received_particles[i].received_states[j].received_frame = 0;
						rpd->received_particles[i].received_states[j].delay = 0;
						rpd->received_particles[i].received_states[j].state = RECEIVED_STATE_UNRECEIVED;
					}
				}
			}
		}
	}

	return rpd;
}
