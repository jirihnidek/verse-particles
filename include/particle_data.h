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

#ifndef PARTICLE_DATA_H_
#define PARTICLE_DATA_H_

#include <verse.h>

#include "types.h"

typedef enum Particle_State {
	PARTICLE_STATE_RESERVED	= 0,
	PARTICLE_STATE_UNBORN	= 1,
	PARTICLE_STATE_ACTIVE	= 2,
	PARTICLE_STATE_DEAD		= 3
} Particle_State;

/**
 * Structure containing reference informations about one particle at one frame.
 */
typedef struct RefParticleState {
	uint16					frame;		/* Current frame */
	enum Particle_State		state;		/* Current state of particle */
	real32					pos[3];		/* Current position of particle */
	real32					vel[3];		/* Current velocity of particle */
} RefParticleState;

/**
 * Structure containing all reference information about one particle
 */
typedef struct RefParticle {
	uint16					id;			/* ID of this particle */
	uint16					born_frame;	/* Frame, when this particle is born */
	uint16					die_frame;	/* Frame, when this particle die */
	struct RefParticleState	*states;	/* Array of particle states */
} RefParticle;

/**
 * Structure containing reference informations about particle simulation
 */
typedef struct RefParticleData {
	char					*dir_name;		/* Name of directory containing files with particle system */
	uint16					particle_count;	/* Count of particles in particle system */
	uint16					frame_count;	/* Duration of particle system in frames */
	struct RefParticle		*particles;		/* Array of particles */
} RefParticleData;


typedef enum Received_State {
	RECEIVED_STATE_RESERVER		= 0,
	RECEIVED_STATE_UNRECEIVED	= 1,
	RECEIVED_STATE_INTIME		= 2,
	RECEIVED_STATE_DELAY		= 3,
	RECEIVED_STATE_AHEAD		= 4
} Received_State;

/**
 * Structure holding information about one received state
 */
typedef struct ReceivedParticleState {
	enum Received_State			state;
	int16						delay;
	int16						received_frame;
	struct RefParticleState		*ref_particle_state;
} ReceivedParticleState;

/**
 * Structure holding information about received particle
 */
typedef struct ReceivedParticle {
	struct ReceivedParticleState	*first_received_state;
	struct ReceivedParticleState	*last_received_state;
	struct ReceivedParticleState	*current_received_state;
	struct ReceivedParticleState	*received_states;
	struct RefParticle				*ref_particle;
} ReceivedParticle;

/**
 * Structure containing received particle data
 */
typedef struct ReceivedParticleData {
	pthread_mutex_t				mutex;
	int16						rec_frame;
	struct ReceivedParticle		*received_particles;
	struct RefParticleData		*ref_particle_data;
} ReceivedParticleData;

struct Client_CTX;

void free_ref_particle_data(struct RefParticleData *pd);
struct RefParticleData *read_ref_particle_data(char *dir_name);

struct RefParticleState *find_ref_particle_state(struct RefParticleData *pd,
		struct RefParticle *ref_particle,
		const int16 frame,
		const real32 pos[3]);
void reset_received_particle_data(struct ReceivedParticleData *rpd);
struct ReceivedParticleData *create_received_particle_data(struct Client_CTX *ctx);
void free_received_particle_data(struct ReceivedParticleData *rpd);

#endif /* PARTICLE_DATA_H_ */
