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
#include <unistd.h>
#include <signal.h>

#include <verse.h>

#include <assert.h>

#include "particle_scene_node.h"
#include "particle_sender_node.h"
#include "particle_node.h"
#include "node.h"
#include "client.h"
#include "lu_table.h"

static struct Client_CTX *ctx = NULL;

static void handle_signal(int sig)
{
	if(sig == SIGINT) {
		if(ctx != NULL) {
			printf("%s() try to terminate connection: %d\n",
					__FUNCTION__, ctx->verse.session_id);
			verse_send_connect_terminate(ctx->verse.session_id);
		} else {
			exit(EXIT_FAILURE);
		}
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
	}
}

static void cb_receive_tag_set_vec3_real32(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id,
		const real32 vec[3])
{
	struct Node *node;
	struct ParticleSenderNode *sender_node;

	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, vec: (%6.3f, %6.3f, %6.3f)\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, vec[0], vec[1], vec[2]);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(sender_node->particle_taggroup_id == taggroup_id &&
					sender_node->pos_tag_id == tag_id)
			{
			}
			break;
		}
	}
}

static void cb_receive_tag_set_uint16(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id,
		const uint16 value)
{
	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, value: %u\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, value);
}

static void cb_receive_tag_set_int16(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id,
		const int16 value)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;

	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, value: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, value);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(scene_node->particle_frame_tag_id == tag_id) {
				static int16 init_value;
				pthread_mutex_lock(&ctx->timer->mutex);
				if(ctx->timer->run == 0) {
					ctx->timer->run = 1;
					init_value = value;
					ctx->timer->tot_frame = value - 1;
				} else if(ctx->timer->tot_frame > ctx->pd->frame_count &&
						value <= init_value)
				{
					init_value = value;
					ctx->timer->tot_frame = value - 1;
				}
				pthread_mutex_unlock(&ctx->timer->mutex);
			}
			break;
		}
	}
}

static void cb_receive_tag_destroy(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id)
{
	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, tag_id: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id);
}

static void cb_receive_tag_create(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id,
		const uint8 type,
		char *name)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;
	struct ParticleNode *particle_node;

	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, tag_id: %d, type: %d, name: %s\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, type, name);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(scene_node->particle_taggroup_id == taggroup_id) {
				if(type == TAG_TYPE_INT16 && strcmp(name, "Particle_Frame") == 0) {
					scene_node->particle_frame_tag_id = tag_id;
					/* Frame start to be send, when animation starts */
				} else if(type == TAG_TYPE_UINT16 && strcmp(name, "Sender_Count") == 0) {
					scene_node->sender_count_tag_id = tag_id;

					verse_send_tag_set_uint16(session_id, DEFAULT_PRIORITY,
							node_id, taggroup_id, tag_id, ctx->sender_count);
				}
			}
			break;
		case SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(sender_node->particle_taggroup_id == taggroup_id) {
				if(type == TAG_TYPE_VEC3_REAL32 && strcmp(name, "Position") == 0) {
					sender_node->pos_tag_id = tag_id;
					if(sender_node->sender != NULL) {
						verse_send_tag_set_vec3_real32(session_id, DEFAULT_PRIORITY,
								node_id, taggroup_id, tag_id, sender_node->sender->pos);
					}
				} else if(type == TAG_TYPE_UINT16 && strcmp(name, "Particle_Count") == 0) {
					sender_node->count_tag_id = tag_id;
					verse_send_tag_set_uint16(session_id, DEFAULT_PRIORITY, node_id,
							taggroup_id, tag_id, ctx->pd->particle_count);
				} else if(type == TAG_TYPE_UINT16 && strcmp(name, "ID") == 0) {
					sender_node->sender_id_tag_id = tag_id;
					if(sender_node->sender != NULL) {
						verse_send_tag_set_uint16(session_id, DEFAULT_PRIORITY,
								node_id, taggroup_id, tag_id, sender_node->sender->id);
					}
				}
			}
			break;
		case PARTICLE_NODE:
			particle_node = (struct ParticleNode*)node;

			if(particle_node->particle_taggroup_id == taggroup_id) {
				if(type == TAG_TYPE_VEC3_REAL32 && strcmp(name, "Position") == 0) {
					particle_node->pos_tag_id = tag_id;
					/* Send position, when animation starts */
				} else if(type == TAG_TYPE_UINT16 && strcmp(name, "ID") == 0) {
					particle_node->particle_id_tag_id = tag_id;
					verse_send_tag_set_uint16(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, tag_id, particle_node->ref_particle->id);
				}
			}
			break;
		}
	}

}

static void cb_receive_taggroup_destroy(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id)
{
	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id);
}

static void cb_receive_taggroup_create(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		char *name)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;
	struct ParticleNode *particle_node;

	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, name: %s\n",
				__FUNCTION__, session_id, node_id, taggroup_id, name);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(strcmp(name, "Particle_Scene") == 0) {
				scene_node->particle_taggroup_id = taggroup_id;

				verse_send_taggroup_subscribe(session_id, DEFAULT_PRIORITY, node_id, taggroup_id);

				verse_send_tag_create(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, TAG_TYPE_INT16, "Particle_Frame");
				verse_send_tag_create(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, TAG_TYPE_UINT16, "Sender_Count");
			}
			break;
		case SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(strcmp(name, "Particle_Sender") == 0) {
				sender_node->particle_taggroup_id = taggroup_id;


				verse_send_taggroup_subscribe(session_id, DEFAULT_PRIORITY, node_id, taggroup_id);

				verse_send_tag_create(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, TAG_TYPE_UINT16, "Particle_Count");
				verse_send_tag_create(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, TAG_TYPE_UINT16, "ID");
				verse_send_tag_create(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, TAG_TYPE_VEC3_REAL32, "Position");
			}
			break;
		case PARTICLE_NODE:
			particle_node = (struct ParticleNode*)node;
			if(strcmp(name, "Particle") == 0) {
				particle_node->particle_taggroup_id = taggroup_id;

				verse_send_taggroup_subscribe(session_id, DEFAULT_PRIORITY, node_id, taggroup_id);

				verse_send_tag_create(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, TAG_TYPE_UINT16, "ID");
				verse_send_tag_create(session_id, DEFAULT_PRIORITY, node_id, taggroup_id, TAG_TYPE_VEC3_REAL32, "Position");
			}
			break;
		}
	}
}

static void cb_receive_node_link(const uint8 session_id,
		const uint32 parent_node_id,
		const uint32 child_node_id)
{
	printf("%s() session_id: %d, parent_node_id: %d, child_node_id: %d\n",
			__FUNCTION__, session_id, parent_node_id, child_node_id);
}

static void cb_receive_node_create(const uint8 session_id,
		const uint32 node_id,
		const uint32 parent_id,
		const uint16 user_id)
{
	printf("%s() session_id: %d, node_id: %d, parent_id: %d, user_id: %d\n",
			__FUNCTION__, session_id, node_id, parent_id, user_id);

	if(user_id != ctx->verse.user_id) {
		return;
	}

	if(parent_id == ctx->verse.avatar_id) {
		if(ctx->verse.particle_scene_node == NULL) {
			/* Create node of particle scene */
			ctx->verse.particle_scene_node = create_particle_scene_node(node_id);

			/* Add node to lookup table*/
			lu_add_item(ctx->verse.lu_table, node_id, ctx->verse.particle_scene_node);

			verse_send_node_link(session_id, DEFAULT_PRIORITY, SCENE_PARENT_NODE_ID, node_id);

			verse_send_node_subscribe(session_id, DEFAULT_PRIORITY, node_id, 0);

			verse_send_taggroup_create(session_id, DEFAULT_PRIORITY, node_id, "Particle_Scene");
		} else if(v_list_count_items(&ctx->verse.particle_scene_node->senders) < (int32)ctx->sender_count) {
			struct ParticleSenderNode *sender_node = create_particle_sender_node(ctx->verse.particle_scene_node, node_id);
			struct Particle_Sender *sender;

			/* Find unassigned sender */
			sender = ctx->senders.first;
			while(sender != NULL) {
				if(sender->sender_node == NULL) {
					break;
				}
				sender = sender->next;
			}

			/* Create reference */
			if(sender != NULL) {
				sender->sender_node = sender_node;
				sender_node->sender = sender;
			}

			/* Add node to lookup table*/
			lu_add_item(ctx->verse.lu_table, node_id, sender_node);

			v_list_add_tail(&ctx->verse.particle_scene_node->senders, sender_node);

			verse_send_node_subscribe(session_id, DEFAULT_PRIORITY, node_id, 0);

			verse_send_node_link(session_id, DEFAULT_PRIORITY, ctx->verse.particle_scene_node->node_id, node_id);

			verse_send_taggroup_create(session_id, DEFAULT_PRIORITY, node_id, "Particle_Sender");
		} else {
			struct ParticleSenderNode *sender_node;
			struct ParticleNode *particle_node;
			int sender_particle_count = 0;

			sender_node = ctx->verse.particle_scene_node->senders.first;
			while(sender_node != NULL) {
				sender_particle_count = v_list_count_items(&sender_node->particles);
				if(sender_particle_count < ctx->pd->particle_count) {
					particle_node = create_particle_node(sender_node, node_id);

					/* Add node to lookup table*/
					lu_add_item(ctx->verse.lu_table, node_id, particle_node);

					particle_node->ref_particle = &ctx->pd->particles[sender_particle_count];

					v_list_add_tail(&sender_node->particles, particle_node);

					verse_send_node_subscribe(session_id, DEFAULT_PRIORITY, node_id, 0);

					verse_send_node_link(session_id, DEFAULT_PRIORITY, sender_node->node_id, node_id);

					verse_send_taggroup_create(session_id, DEFAULT_PRIORITY, node_id, "Particle");

					break;
				}
				sender_node = sender_node->next;
			}
		}
	}
}

static void cb_receive_node_destroy(const uint8 session_id,
		const uint32 node_id)
{
	printf("%s() session_id: %d, node_id: %d\n",
			__FUNCTION__, session_id, node_id);
}

static void cb_receive_connect_accept(const uint8 session_id,
      const uint16 user_id,
      const uint32 avatar_id)
{
	uint32 i, j;

	printf("%s() session_id: %d, user_id: %d, avatar_id: %d\n",
			__FUNCTION__, session_id, user_id, avatar_id);

	ctx->verse.avatar_id = avatar_id;
	ctx->verse.user_id = user_id;

	/* Subscribe to avatar node */
	verse_send_node_subscribe(session_id, DEFAULT_PRIORITY, avatar_id, 0);

	/* Create new node (particle scene node) */
	verse_send_node_create(session_id, DEFAULT_PRIORITY);

	for(i=0; i<ctx->sender_count; i++) {
		/* Create new node (particle sender) */
		verse_send_node_create(session_id, DEFAULT_PRIORITY);
		/* Create new node (particle) */
		for(j=0; j<ctx->pd->particle_count; j++) {
			verse_send_node_create(session_id, DEFAULT_PRIORITY);
		}
	}
}

static void cb_receive_connect_terminate(const uint8 session_id,
		const uint8 error_num)
{
	printf("%s() session_id: %d, error_num: %d\n",
			__FUNCTION__, session_id, error_num);
	exit(EXIT_SUCCESS);
}

static void cb_receive_user_authenticate(const uint8 session_id,
		const char *username,
		const uint8 auth_methods_count,
		const uint8 *methods)
{
	static int attempts=0;	/* Store number of authentication attempt for this session. */
	char name[64];
	char *password;
	int i, is_passwd_supported=0;

	static char *my_user_name = "a";
	static char *my_password = "a";

	/*
	static char *my_user_name = "";
	static char *my_password = "";
	*/

	/* Debug print */
	printf("%s() username: %s, auth_methods_count: %d, methods: ",
			__FUNCTION__, username, auth_methods_count);

	for(i=0; i<auth_methods_count; i++) {
		printf("%d, ", methods[i]);
		if(methods[i]==UA_METHOD_PASSWORD)
			is_passwd_supported = 1;
	}
	printf("\n");

	/* Get username, when it is requested */
	if(username == NULL) {
		if(strlen(my_user_name)==0) {
			printf("Username: ");
			scanf("%s", name);
			attempts = 0;	/* Reset counter of auth. attempt. */
			verse_send_user_authenticate(session_id, name, UA_METHOD_NONE, NULL);
		} else {
			verse_send_user_authenticate(session_id, my_user_name, UA_METHOD_NONE, NULL);
		}
	} else {
		if(is_passwd_supported==1) {
			strcpy(name, username);
			/* Print this warning, when previous authentication attempt failed. */
			if(attempts>0)
				printf("Permission denied, please try again.\n");
			if(strlen(my_password)==0) {
				/* Get password from user */
				password = getpass("Password: ");
				attempts++;
				verse_send_user_authenticate(session_id, name, UA_METHOD_PASSWORD, password);
			} else {
				verse_send_user_authenticate(session_id, name, UA_METHOD_PASSWORD, my_password);
			}
		} else {
			printf("ERROR: Verse server does not support password authentication method\n");
		}
	}
}

static void register_cb_func_particle_sender(void)
{
	/* Register callback functions */
	register_receive_user_authenticate(cb_receive_user_authenticate);
	register_receive_connect_accept(cb_receive_connect_accept);
	register_receive_connect_terminate(cb_receive_connect_terminate);

	register_receive_node_create(cb_receive_node_create);
	register_receive_node_destroy(cb_receive_node_destroy);
	register_receive_node_link(cb_receive_node_link);

	register_receive_taggroup_create(cb_receive_taggroup_create);
	register_receive_taggroup_destroy(cb_receive_taggroup_destroy);
	register_receive_tag_create(cb_receive_tag_create);
	register_receive_tag_destroy(cb_receive_tag_destroy);

	register_receive_tag_set_uint16(cb_receive_tag_set_uint16);
	register_receive_tag_set_int16(cb_receive_tag_set_int16);

	register_receive_tag_set_vec3_real32(cb_receive_tag_set_vec3_real32);
}

/**
 * When receiver set up trigger, then sender sends particle position
 * each frame.
 */
static void verse_send_data(void)
{
	pthread_mutex_lock(&ctx->timer->mutex);

	if(ctx->timer->run == 1) {

		/* Send current frame */
		if(ctx->timer->tot_frame < ctx->pd->frame_count) {
			verse_send_tag_set_int16(ctx->verse.session_id,
					DEFAULT_PRIORITY,
					ctx->verse.particle_scene_node->node_id,
					ctx->verse.particle_scene_node->particle_taggroup_id,
					ctx->verse.particle_scene_node->particle_frame_tag_id,
					ctx->timer->tot_frame);
		}

		/* Send position for current frame */
		if(ctx->timer->tot_frame >=0 &&
				ctx->timer->tot_frame < ctx->pd->frame_count)
		{
			struct ParticleSceneNode *scene_node = ctx->verse.particle_scene_node;
			struct ParticleSenderNode *sender_node;
			struct ParticleNode *particle_node;

			/* For all senders ... */
			for(sender_node = scene_node->senders.first; sender_node != NULL; sender_node = sender_node->next) {
				/* For all particles of sender ... */
				for(particle_node = sender_node->particles.first; particle_node !=NULL; particle_node = particle_node->next) {
					/* Send all active particles */
					if(particle_node->ref_particle->states[ctx->timer->frame].state == PARTICLE_STATE_ACTIVE) {
						verse_send_tag_set_vec3_real32(ctx->verse.session_id,
								DEFAULT_PRIORITY,
								particle_node->node_id,
								particle_node->particle_taggroup_id,
								particle_node->pos_tag_id,
								particle_node->ref_particle->states[ctx->timer->frame].pos);
					}
				}
			}
		}
	}

	pthread_mutex_unlock(&ctx->timer->mutex);
}

int particle_sender_loop(struct Client_CTX *ctx_)
{
	int ret;

	ctx = ctx_;

	/* Handle SIGINT signal. The handle_signal function will try to terminate
	 * connection. */
	signal(SIGINT, handle_signal);

	register_cb_func_particle_sender();

	if((ret = verse_send_connect_request(ctx->verse.server_name, "12345", DGRAM_SEC_NONE ,&ctx->verse.session_id))!=VC_SUCCESS) {
		printf("ERROR: %s\n", verse_strerror(ret));
		return 0;
	}

	/* Never ending loop */
	while(1) {
		/* usleep(1000000/ctx->verse.fps); */
		sem_wait(&ctx->timer_sem);
		verse_callback_update(ctx->verse.session_id);
		verse_send_data();
	}

	return 1;
}
