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

#include "particle_scene_node.h"
#include "particle_sender_node.h"
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
			vrs_send_connect_terminate(ctx->verse.session_id);
		} else {
			exit(EXIT_FAILURE);
		}
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
	}
}

static void cb_receive_layer_unset_value(const uint8_t session_id,
		const uint32_t node_id,
		const uint16_t layer_id,
		const uint32_t item_id)
{
	struct Node *node;
	struct ParticleSenderNode *sender_node;
	struct Particle_Sender *sender;

	printf("%s() session_id: %u, node_id: %u, layer_id: %u, item_id: %u\n",
				__FUNCTION__, session_id, node_id, layer_id, item_id);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL &&
			node->type == PARTICLE_SENDER_NODE) {
		struct ReceivedParticle *rec_particle;
		sender_node = (struct ParticleSenderNode*)node;
		sender = sender_node->sender;

		if(layer_id == sender_node->particle_layer_id) {

			pthread_mutex_lock(&sender_node->sender->rec_pd->mutex);

			rec_particle = &sender->rec_pd->received_particles[item_id];
			rec_particle->first_received_state = NULL;
			rec_particle->last_received_state = NULL;
			rec_particle->current_received_state = NULL;

			pthread_mutex_unlock(&sender_node->sender->rec_pd->mutex);
		}

	} else {
		printf("ERROR: Sender node not found\n");
	}
}

static void cb_receive_layer_set_value(const uint8_t session_id,
		const uint32_t node_id,
		const uint16_t layer_id,
		const uint32_t item_id,
		const uint8_t data_type,
		const uint8_t count,
		const void *value)
{
	struct Node *node;
	struct ParticleSenderNode *sender_node;
	struct Particle_Sender *sender;
	struct RefParticleState *ref_state;
	uint16 current_frame;

	printf("%s() session_id: %u, node_id: %u, layer_id: %u, item_id: %u, data_type: %u, count: %u, value: %p\n",
				__FUNCTION__, session_id, node_id, layer_id, item_id, data_type, count, value);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL && node->type == PARTICLE_SENDER_NODE) {
		sender_node = (struct ParticleSenderNode*)node;
		sender = sender_node->sender;

		pthread_mutex_lock(&sender_node->sender->timer->mutex);
		current_frame = sender_node->sender->timer->frame;
		pthread_mutex_unlock(&sender_node->sender->timer->mutex);

		pthread_mutex_lock(&sender_node->sender->rec_pd->mutex);

		/* Find reference state */
		ref_state = find_ref_particle_state(ctx->pd,
				&ctx->pd->particles[item_id],
				sender_node->sender->rec_pd->rec_frame,
				(real32*)value);

		/* Was reference state found? */
		if(ref_state != NULL) {
			struct ReceivedParticleState *rec_state;
			struct ReceivedParticle *rec_particle;

			rec_state = &sender->rec_pd->received_particles[item_id].received_states[ref_state->frame];
			rec_particle = &sender->rec_pd->received_particles[item_id];

			/* Set up first, last and current received state */
			if(rec_particle->first_received_state == NULL) {
				rec_particle->first_received_state = rec_state;
				rec_particle->last_received_state = rec_state;
			} else {
				if(rec_particle->first_received_state->ref_particle_state->frame > rec_state->ref_particle_state->frame) {
					rec_particle->first_received_state = rec_state;
				}
				if(rec_particle->last_received_state->ref_particle_state->frame < rec_state->ref_particle_state->frame) {
					rec_particle->last_received_state = rec_state;
				}
			}

			/* This state is the current received */
			rec_particle->current_received_state = rec_state;

			/* At this frame was particle received */
			rec_state->received_frame = current_frame;
			/* Set up delay of receiving */
			rec_state->delay = current_frame - ref_state->frame;

			/* Set up state according delay */
			if(rec_state->delay == 0 || rec_state->delay == 1) {
				rec_state->state = RECEIVED_STATE_INTIME;
			} else if( rec_state->delay > 1) {
				rec_state->state = RECEIVED_STATE_DELAY;
			} else {
				rec_state->state = RECEIVED_STATE_AHEAD;
			}

		} else {
			printf("ERROR: Reference particle state not found\n");
		}

		pthread_mutex_unlock(&sender_node->sender->rec_pd->mutex);

	} else {
		printf("ERROR: Sender node not found\n");
	}
}

static void cb_receive_layer_create(const uint8 session_id,
		const uint32 node_id,
		const uint16 parent_layer_id,
		const uint16 layer_id,
		const uint8 data_type,
		const uint8 count,
		const uint16 custom_type)
{
	struct Node *node;
	struct ParticleSenderNode *sender_node;

	printf("%s() session_id: %u, node_id: %u, layer_id: %u, parent_layer_id: %u, data_type: %u, count: %u, custom_type: %u\n",
				__FUNCTION__, session_id, node_id, layer_id, parent_layer_id, data_type, count, custom_type);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node->type == PARTICLE_SENDER_NODE) {
		if(custom_type == PARTICLE_POS_LAYER) {
			sender_node = (struct ParticleSenderNode*)node;
			sender_node->particle_layer_id = layer_id;

			vrs_send_layer_subscribe(session_id, VRS_DEFAULT_PRIORITY,
					node_id, layer_id, 0, 0);
		}
	}
}

static void _frame_received(struct ParticleSenderNode *sender_node,
		int16 value)
{
	/* Start timer, when first frame value is received */
	pthread_mutex_lock(&sender_node->sender->timer->mutex);
	if(sender_node->sender->timer->run == 0) {
		sender_node->sender->timer->run = 1;
		sender_node->sender->timer->tot_frame = value;
	}
	pthread_mutex_unlock(&sender_node->sender->timer->mutex);

	/* Save received frame */
	pthread_mutex_lock(&sender_node->sender->rec_pd->mutex);
	sender_node->sender->rec_pd->rec_frame = value;
	pthread_mutex_unlock(&sender_node->sender->rec_pd->mutex);
}

static void cb_receive_tag_set_value(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id,
		const uint8_t data_type,
		const uint8_t count,
		const void *value)
{
	struct Node *node;
	/* struct ParticleSceneNode *scene_node; */
	/* struct ParticleNode *particle_node; */
	struct ParticleSenderNode *sender_node;

	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, data_type: %d, count: %d, value: %p\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, data_type, count, value);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			/* scene_node = (struct ParticleSceneNode *)node; */

			/* TODO: do something useful here */
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode *)node;

			/* Was current frame received? */
			if(sender_node->particle_taggroup_id == taggroup_id) {
				if(sender_node->particle_frame_tag_id == tag_id) {
					_frame_received(sender_node, *(int16*)value);
				}
			}
			break;
		}
	} else {
		printf("ERROR: node not found\n");
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
		const uint8 data_type,
		const uint8 count,
		const uint16 custom_type)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;
	/* struct ParticleNode *particle_node; */

	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, tag_id: %d, data_type: %d, count: %d, custom_type: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, data_type, count, custom_type);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(scene_node->particle_taggroup_id == taggroup_id) {
				if(data_type == VRS_VALUE_TYPE_UINT16 &&
						custom_type == SENDER_COUNT_TAG)
				{
					scene_node->sender_count_tag_id = tag_id;
				}
			}
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(sender_node->particle_taggroup_id == taggroup_id)
			{
				if(data_type == VRS_VALUE_TYPE_UINT16 &&
						custom_type == PARTICLE_FRAME_TAG)
				{
					sender_node->particle_frame_tag_id = tag_id;
				}
				else if(data_type == VRS_VALUE_TYPE_REAL32 &&
						count == 3 &&
						custom_type == POSITION_TAG)
				{
					sender_node->pos_tag_id = tag_id;
				}
				else if(data_type == VRS_VALUE_TYPE_UINT16 &&
						custom_type == PARTICLE_COUNT_TAG)
				{
					sender_node->count_tag_id = tag_id;
				}
				else if(data_type == VRS_VALUE_TYPE_UINT16 &&
						custom_type == SENDER_ID_TAG)
				{
					sender_node->sender_id_tag_id = tag_id;
				}
			}
			break;
#if 0
		case PARTICLE_NODE:
			particle_node = (struct ParticleNode*)node;

			if(particle_node->particle_taggroup_id == taggroup_id) {
				if(data_type == VRS_VALUE_TYPE_REAL32 &&
						count == 3 &&
						custom_type == POSITION_TAG)
				{
					particle_node->pos_tag_id = tag_id;
				} else if(data_type == VRS_VALUE_TYPE_UINT16 &&
						custom_type == PARTICLE_ID_TAG)
				{
					particle_node->particle_id_tag_id = tag_id;
				}
			}
			break;
#endif
		}
	} else {
		printf("ERROR: node not found\n");
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
		const uint16 custom_type)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;

	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, custom_type: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id, custom_type);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(scene_node->node_id == node_id &&
					custom_type == PARTICLE_SCENE_TG)
			{
				scene_node->particle_taggroup_id = taggroup_id;
				/* Subscribe to this tag group */
				vrs_send_taggroup_subscribe(session_id, VRS_DEFAULT_PRIORITY,
						node_id, taggroup_id, 0, 0);
			}
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(custom_type == PARTICLE_SENDER_TG) {
				sender_node->particle_taggroup_id = taggroup_id;
				/* Subscribe to this tag group */
				vrs_send_taggroup_subscribe(session_id, VRS_DEFAULT_PRIORITY,
						node_id, taggroup_id, 0, 0);
			}
			break;
		}
	} else {
		printf("ERROR: node not found\n");
	}

}

static void cb_receive_node_create(const uint8 session_id,
		const uint32 node_id,
		const uint32 parent_id,
		const uint16 user_id,
		const uint16 custom_type)
{
	printf("%s() session_id: %d, node_id: %d, parent_id: %d, user_id: %d, custom_type: %d\n",
			__FUNCTION__, session_id, node_id, parent_id, user_id, custom_type);

	if(user_id != ctx->verse.user_id) {
		return;
	}

	switch (custom_type) {
	case PARTICLE_SCENE_NODE:
		if(ctx->verse.particle_scene_node == NULL &&
				parent_id == VRS_SCENE_PARENT_NODE_ID ) {
			/* Create node of particle scene */
			ctx->verse.particle_scene_node = create_particle_scene_node(node_id);

			/* Add node to lookup table*/
			lu_add_item(ctx->verse.lu_table, node_id, ctx->verse.particle_scene_node);

			/* Subscribe to this node */
			vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, 0, 0);
		}
		break;
	case PARTICLE_SENDER_NODE:
		if(parent_id == ctx->verse.particle_scene_node->node_id &&
			v_list_count_items(&ctx->verse.particle_scene_node->senders) < (int32)ctx->sender_count)
		{
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

			/* Set up references */
			if(sender != NULL) {
				printf("Info: setting up references\n");
				sender_node->sender = sender;
				sender->sender_node = sender_node;
			} else {
				printf("Error: no remaining free sender\n");
			}

			/* Add node to lookup table*/
			lu_add_item(ctx->verse.lu_table, node_id, sender_node);

			v_list_add_tail(&ctx->verse.particle_scene_node->senders, sender_node);

			vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, 0, 0);
		}
		break;
	}
}

static void cb_receive_node_destroy(const uint8 session_id,
		const uint32 node_id)
{
	printf("%s() session_id: %d, node_id: %d\n",
			__FUNCTION__, session_id, node_id);
	/* TODO: add some usefull code here  */
}

static void cb_receive_node_link(const uint8 session_id,
		const uint32 parent_node_id,
		const uint32 child_node_id)
{
	printf("%s() session_id: %d, parent_node_id: %d, child_node_id: %d\n",
			__FUNCTION__, session_id, parent_node_id, child_node_id);
	/* TODO: add some usefull code here  */
}

static void cb_receive_connect_accept(const uint8 session_id,
      const uint16 user_id,
      const uint32 avatar_id)
{
	printf("%s() session_id: %d, user_id: %d, avatar_id: %d\n",
			__FUNCTION__, session_id, user_id, avatar_id);

	ctx->verse.avatar_id = avatar_id;
	ctx->verse.user_id = user_id;

	/* Subscribe to avatar node */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, avatar_id, 0, 0);

	/* Subscribe to parent of scenes */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, VRS_SCENE_PARENT_NODE_ID, 0, 0);

}

static void cb_receive_connect_terminate(const uint8 session_id,
		const uint8 error_num)
{
	printf("%s() session_id: %d, error_num: %d\n",
			__FUNCTION__, session_id, error_num);
	exit(0);
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
		if(methods[i]==VRS_UA_METHOD_PASSWORD)
			is_passwd_supported = 1;
	}
	printf("\n");

	/* Get username, when it is requested */
	if(username == NULL) {
		if(strlen(my_user_name)==0) {
			printf("Username: ");
			scanf("%s", name);
			attempts = 0;	/* Reset counter of auth. attempt. */
			vrs_send_user_authenticate(session_id, name, VRS_UA_METHOD_NONE, NULL);
		} else {
			vrs_send_user_authenticate(session_id, my_user_name, VRS_UA_METHOD_NONE, NULL);
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
				vrs_send_user_authenticate(session_id, name, VRS_UA_METHOD_PASSWORD, password);
			} else {
				vrs_send_user_authenticate(session_id, name, VRS_UA_METHOD_PASSWORD, my_password);
			}
		} else {
			printf("ERROR: Verse server does not support password authentication method\n");
		}
	}
}

static void register_cb_func_particle_receiver(void)
{
	/* System callbacks */
	vrs_register_receive_user_authenticate(cb_receive_user_authenticate);
	vrs_register_receive_connect_accept(cb_receive_connect_accept);
	vrs_register_receive_connect_terminate(cb_receive_connect_terminate);

	/* Node callbacks */
	vrs_register_receive_node_create(cb_receive_node_create);
	vrs_register_receive_node_destroy(cb_receive_node_destroy);
	vrs_register_receive_node_link(cb_receive_node_link);

	/* TagGroup callbacks */
	vrs_register_receive_taggroup_create(cb_receive_taggroup_create);
	vrs_register_receive_taggroup_destroy(cb_receive_taggroup_destroy);

	/* Tag callbacks */
	vrs_register_receive_tag_create(cb_receive_tag_create);
	vrs_register_receive_tag_destroy(cb_receive_tag_destroy);
	vrs_register_receive_tag_set_value(cb_receive_tag_set_value);

	/* Layer callbacks */
	vrs_register_receive_layer_create(cb_receive_layer_create);
	vrs_register_receive_layer_set_value(cb_receive_layer_set_value);
	vrs_register_receive_layer_unset_value(cb_receive_layer_unset_value);
}

void *particle_receiver_loop(void *arg)
{
	int ret;

	ctx = (struct Client_CTX*)arg;

	/* Handle SIGINT signal. The handle_signal function will try to terminate
	 * connection. */
	signal(SIGINT, handle_signal);

	register_cb_func_particle_receiver();

	if((ret = vrs_send_connect_request(ctx->verse.server_name, "12345",
			VRS_SEC_DATA_NONE ,&ctx->verse.session_id))!=VRS_SUCCESS) {
		printf("ERROR: %s\n", vrs_strerror(ret));
		return 0;
	}

	/* Never ending loop */
	while(1) {
		sem_wait(&ctx->timer_sem);
		vrs_callback_update(ctx->verse.session_id);
	}

	return NULL;
}
