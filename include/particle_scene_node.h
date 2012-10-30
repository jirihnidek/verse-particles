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

#ifndef PARTICLE_SCENE_H_
#define PARTICLE_SCENE_H_

#include <verse.h>
#include <v_list.h>

#define PSCE_FRAME_TAG_NAME			"Particle_Frame"
#define PSCE_FRAME_TAG_TYPE			TAG_TYPE_INT32

#define PSCE_SENDER_COUNT_TAG_NAME	"Particle_Sender_Count"
#define PSCE_SENDER_COUNT_TAG_TYPE	TAG_TYPE_UINT16

/**
 * This structure contains information about scene node. It should be parent
 * of all particle senders.
 */
typedef struct ParticleSceneNode {
	struct ParticleSceneNode	*prev, *next;
	uint8						type;
	uint32						node_id;
	uint16						particle_taggroup_id;
	uint16						particle_frame_tag_id;
	int16						received_frame;
	uint16						sender_count_tag_id;
	int16						particle_tag_id;
	struct VListBase			senders;
} ParticleSceneNode;

struct ParticleSceneNode *create_particle_scene_node(uint32 node_id);

#endif /* PARTICLE_SCENE_H_ */
