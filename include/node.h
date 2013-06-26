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

#ifndef NODE_H_
#define NODE_H_

/* Custom types of nodes */
#define PARTICLE_SCENE_NODE		100
#define PARTICLE_SENDER_NODE	101

/* Custom types of taggroups */
#define PARTICLE_SCENE_TG		200
#define PARTICLE_SENDER_TG		201
#define PARTICLE_TG				202

/* Custom types of tags */
#define PARTICLE_FRAME_TAG 		300
#define SENDER_COUNT_TAG		301
#define POSITION_TAG			302
#define PARTICLE_COUNT_TAG		303
#define SENDER_ID_TAG			304
#define PARTICLE_ID_TAG			305

/* Custom type of layers */
#define PARTICLE_POS_LAYER		400

/**
 * This structure contains informations about verse node.
 */
typedef struct Node {
	struct ParticleNode			*prev, *next;			/** Linked list stuff */
	uint8						type;					/* type of node */
	uint32						node_id;				/* ID of node */
} Node;

#endif /* NODE_H_ */
