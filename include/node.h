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
#define PARTICLE_NODE			102

/* Custom types of taggroups */
#define PARTICLE_SCENE			200
#define PARTICLE_SENDER			201
#define PARTICLE				202

/* Custom types of tags */
#define PARTICLE_FRAME 			300
#define SENDER_COUNT			301
#define POSITION				302
#define PARTICLE_COUNT			303
#define SENDER_ID				304
#define PARTICLE_ID				305

/* Custom type of layers */
#define PARTICLES				400

/**
 * This structure contains informations about verse node.
 */
typedef struct Node {
	struct ParticleNode			*prev, *next;			/** Linked list stuff */
	uint8						type;					/* type of node */
	uint32						node_id;				/* ID of node */
} Node;

#endif /* NODE_H_ */
