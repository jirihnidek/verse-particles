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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <pthread.h>
#include <semaphore.h>

#include <verse.h>

#include "list.h"
#include "types.h"
#include "particle_data.h"
#include "display_glut.h"
#include "particle_scene_node.h"
#include "timer.h"
#include "sender.h"

#define VC_DGRAM_SEC_DTLS		1
#define VC_MAKE_SCREENCAST		2

#define DEFAULT_FPS	25

typedef enum ClientType {
	CLIENT_NONE		= 1,
	CLIENT_RECEIVER	= 2,
	CLIENT_SENDER	= 3
} ClientType;

/**
 * Verse specific stuff
 */
typedef struct VerseData {
	char						*server_name;
	uint8						session_id;
	uint32						avatar_id;
	uint16						user_id;
	struct ParticleSceneNode	*particle_scene_node;
	struct LookUp_Table			*lu_table;
	uint32						fps;
} VerseData;

/**
 * Structure storing information about Verse client
 */
typedef struct Client_CTX {
	uint8						flags;
	enum ClientType				client_type;
	struct VerseData			verse;
	struct RefParticleData		*pd;
	struct ParticleDisplay		*display;
	struct Timer				*timer;
	struct VListBase			senders;
	uint32						sender_count;
	pthread_t					timer_thread;
	sem_t						timer_sem;
	pthread_t					receiver_thread;
} Client_CTX;

#endif /* CLIENT_H_ */
