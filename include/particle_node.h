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

#ifndef PARTICLE_H_
#define PARTICLE_H_

#include <verse.h>

#include "particle_sender_node.h"
#include "particle_data.h"

/**
 * This structure contains informations about node that represent one particle.
 */
typedef struct ParticleNode {
	struct ParticleNode			*prev, *next;			/** Linked list stuff */
	uint8						type;					/* type of node */
	uint32						node_id;				/* ID of node */
	uint16						particle_taggroup_id;	/* ID of tag group containing tag with position */
	uint16						pos_tag_id;				/* ID of tag containing position of particle */
	uint16						particle_id_tag_id;		/* ID of tag containing ID of particle */

	real32						pos[3];					/* Last received position */
	uint32						frame;					/* Last received frame */

	struct ParticleSenderNode	*sender;				/* Parent of this node */
	struct RefParticle			*ref_particle;			/* Pointer at structure containing reference data for this particle */
	struct ReceivedParticle		*rec_particle;			/* Pointer at structure containing received data */
} ParticleNode;

struct ParticleNode *create_particle_node(struct ParticleSenderNode *sender_node,
		uint32 node_id);

#endif /* PARTICLE_H_ */
