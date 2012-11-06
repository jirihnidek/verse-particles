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

#include <verse.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "client.h"
#include "sender.h"
#include "time.h"
#include "particle_data.h"

/**
 * \brief This function create sender structure
 */
static struct Particle_Sender *create_particle_sender(real32 pos[3], uint16 id)
{
	struct Particle_Sender *sender = NULL;

	sender = (struct Particle_Sender*)malloc(sizeof(struct Particle_Sender));

	if(sender != NULL) {
		sender->pos[0] = pos[0];
		sender->pos[1] = pos[1];
		sender->pos[2] = pos[2];
		sender->id = id;

		sender->sender_node = NULL;

		sender->timer = create_timer();

		sender->rec_pd = NULL;
	}

	return sender;
}

/**
 * \brief This function create linked list of senders in client ctx
 */
void create_senders(struct Client_CTX *ctx)
{
	struct Particle_Sender *sender;
	int i, j, id = 0, side = sqrt(ctx->sender_count);
	real32 pos[3];

	for(i=0; i<side; i++) {
		for(j=0; j<side; j++) {
			pos[0] = 0.0 - 40.0*i;
			pos[1] = 0.0 + 40.0*j;
			pos[2] = 0.0;
			sender = create_particle_sender(pos, id);
			if(sender != NULL) {
				if(ctx->client_type == CLIENT_RECEIVER) {
					sender->rec_pd = create_received_particle_data(ctx);
				} else {
					sender->rec_pd = NULL;
				}
			}
			id++;

			v_list_add_tail(&ctx->senders, sender);
		}
	}
}
