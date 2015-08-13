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
#include "node.h"
#include "client.h"
#include "lu_table.h"

static struct Client_CTX *ctx = NULL;

#define NO_DEBUG_PRINT 1

static void handle_signal(int sig)
{
	if(sig == SIGINT) {
		if(ctx != NULL) {
			printf("%s() try to terminate connection: %d\n",
					__FUNCTION__, ctx->verse.session_id);
			/* Try to delete particle sender node */
			vrs_send_node_destroy(ctx->verse.session_id, VRS_DEFAULT_PRIORITY,
					ctx->sender->sender_node->node_id);
			/* Terminate connection */
			vrs_send_connect_terminate(ctx->verse.session_id);
		} else {
			exit(EXIT_FAILURE);
		}
		/* Reset signal handling to default behavior */
		signal(SIGINT, SIG_DFL);
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
#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %u, node_id: %u, layer_id: %u, item_id: %u, data_type: %u, count: %u, value: %p\n",
				__FUNCTION__, session_id, node_id, layer_id, item_id, data_type, count, value);
#else
	(void)session_id;
	(void)node_id;
	(void)layer_id;
	(void)item_id;
	(void)data_type;
	(void)count;
	(void)value;
#endif
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

#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %u, node_id: %u, layer_id: %u, parent_layer_id: %u, data_type: %u, count: %u, custom_type: %u\n",
				__FUNCTION__, session_id, node_id, layer_id, parent_layer_id, data_type, count, custom_type);
#else
	(void)session_id;
	(void)parent_layer_id;
	(void)data_type;
	(void)count;
#endif

	node = lu_find(ctx->verse.lu_table, node_id);

	/* When this is layer of particles, then remember ID of this layer */
	if(node->type == PARTICLE_SENDER_NODE) {
		if(custom_type == PARTICLE_POS_LAYER) {
			sender_node = (struct ParticleSenderNode*)node;
			sender_node->particle_layer_id = layer_id;
		}
	}
}

static void cb_receive_tag_set_value(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id,
		const uint8 data_type,
		const uint8 count,
		const void *value)
{
	struct Node *node;
	struct ParticleSenderNode *sender_node;
	/*struct ParticleSceneNode *scene_node;*/

#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, type: %d, count %d, data: %p\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, data_type, count, value);
#else
	(void)session_id;
	(void)data_type;
	(void)count;
	(void)value;
#endif

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(sender_node->particle_taggroup_id == taggroup_id &&
					sender_node->pos_tag_id == tag_id)
			{
				/* TODO: set position */
			}

			if(sender_node->particle_frame_tag_id == tag_id) {
				/* TODO: do something here */
			}
			break;
		case PARTICLE_SCENE_NODE:
			/* scene_node = (struct ParticleSceneNode *)node;
			TODO: add something useful here */
			break;
		}
	}
}

static void cb_receive_tag_destroy(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id)
{
#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, tag_id: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id);
#else
	(void)session_id;
	(void)node_id;
	(void)taggroup_id;
	(void)tag_id;
#endif
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

#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, tag_id: %d, data_type: %d, count: %d, custom_type: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, data_type, count, custom_type);
#endif

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(scene_node->particle_taggroup_id == taggroup_id) {
				if(data_type == VRS_VALUE_TYPE_UINT16 &&
						count == 1 &&
						custom_type == SENDER_COUNT_TAG)
				{
					scene_node->sender_count_tag_id = tag_id;
				}
			}
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(sender_node->particle_taggroup_id == taggroup_id) {
				if(data_type == VRS_VALUE_TYPE_UINT16 &&
						count == 1 &&
						custom_type == PARTICLE_FRAME_TAG)
				{
					/* Save ID of Tag containing Frame */
					sender_node->particle_frame_tag_id = tag_id;
					/* Start sending of particles */
					pthread_mutex_lock(&sender_node->sender->timer->mutex);
					if(sender_node->sender->timer->run == 0) {
						sender_node->sender->timer->run = 1;
						sender_node->sender->timer->tot_frame = -25;
					}
					pthread_mutex_unlock(&sender_node->sender->timer->mutex);
				}
				else if(data_type == VRS_VALUE_TYPE_REAL32 &&
						count == 3 &&
						custom_type == POSITION_TAG)
				{
					/* Save ID of Tag containing position of sender */
					sender_node->pos_tag_id = tag_id;
					if(sender_node->sender != NULL) {
						vrs_send_tag_set_value(session_id, VRS_DEFAULT_PRIORITY,
								node_id, taggroup_id, tag_id, data_type, count, sender_node->sender->pos);
					}
				}
				else if(data_type == VRS_VALUE_TYPE_UINT16 &&
						count == 1 &&
						custom_type == PARTICLE_COUNT_TAG)
				{
					/* Save ID of Tag containing count of particles of this sender */
					sender_node->count_tag_id = tag_id;
					vrs_send_tag_set_value(session_id, VRS_DEFAULT_PRIORITY, node_id,
							taggroup_id, tag_id, data_type, count, &ctx->pd->particle_count);
				}
				else if(data_type == VRS_VALUE_TYPE_UINT16 && count == 1
						&& custom_type == SENDER_ID_TAG)
				{
					/* Save ID of Tag containing ID od Sender */
					sender_node->sender_id_tag_id = tag_id;
					if(sender_node->sender != NULL) {
						vrs_send_tag_set_value(session_id, VRS_DEFAULT_PRIORITY,
								node_id, taggroup_id, tag_id, data_type, count, &sender_node->sender->id);
					}
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
#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id);
#else
	(void)session_id;
	(void)node_id;
	(void)taggroup_id;
#endif
}

static void cb_receive_taggroup_create(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 custom_type)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;

#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, custom_type: %d\n",
				__FUNCTION__, session_id, node_id, taggroup_id, custom_type);
#endif

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(custom_type == PARTICLE_SCENE_TG) {
				scene_node->particle_taggroup_id = taggroup_id;

				vrs_send_taggroup_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, taggroup_id, 0, 0);

				vrs_send_tag_create(session_id, VRS_DEFAULT_PRIORITY,
						node_id, taggroup_id, VRS_VALUE_TYPE_UINT16, 1, SENDER_COUNT_TAG);
			}
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(custom_type == PARTICLE_SENDER_TG) {
				sender_node->particle_taggroup_id = taggroup_id;

				vrs_send_taggroup_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, taggroup_id, 0, 0);

				vrs_send_tag_create(session_id, VRS_DEFAULT_PRIORITY,
						node_id, taggroup_id, VRS_VALUE_TYPE_UINT16, 1, PARTICLE_FRAME_TAG);
				vrs_send_tag_create(session_id, VRS_DEFAULT_PRIORITY,
						node_id, taggroup_id, VRS_VALUE_TYPE_UINT16, 1, PARTICLE_COUNT_TAG);
				vrs_send_tag_create(session_id, VRS_DEFAULT_PRIORITY,
						node_id, taggroup_id, VRS_VALUE_TYPE_UINT16, 1, SENDER_ID_TAG);
				vrs_send_tag_create(session_id, VRS_DEFAULT_PRIORITY,
						node_id, taggroup_id, VRS_VALUE_TYPE_REAL32, 3, POSITION_TAG);
			}
			break;
		}
	}
}

static void cb_receive_node_link(const uint8 session_id,
		const uint32 parent_node_id,
		const uint32 child_node_id)
{
#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, parent_node_id: %d, child_node_id: %d\n",
			__FUNCTION__, session_id, parent_node_id, child_node_id);
#else
	(void)session_id;
	(void)parent_node_id;
	(void)child_node_id;
#endif
}

static void cb_receive_node_create(const uint8 session_id,
		const uint32 node_id,
		const uint32 parent_id,
		const uint16 user_id,
		const uint16 custom_type)
{
	struct ParticleSenderNode *sender_node;
	struct Particle_Sender *sender;

#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, node_id: %d, parent_id: %d, user_id: %d, custom_type: %d\n",
			__FUNCTION__, session_id, node_id, parent_id, user_id, custom_type);
#endif

	if(user_id != ctx->verse.user_id) {
		return;
	}

	if(parent_id == ctx->verse.avatar_id) {
		switch(custom_type) {
		case PARTICLE_SCENE_NODE:
			if(ctx->verse.particle_scene_node == NULL) {
				/* Create node of particle scene */
				ctx->verse.particle_scene_node = create_particle_scene_node(node_id);

				/* Add node to lookup table*/
				lu_add_item(ctx->verse.lu_table, node_id, ctx->verse.particle_scene_node);

				vrs_send_node_link(session_id, VRS_DEFAULT_PRIORITY, VRS_SCENE_PARENT_NODE_ID, node_id);
				vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, 0, 0);
				vrs_send_taggroup_create(session_id, VRS_DEFAULT_PRIORITY, node_id, PARTICLE_SCENE_TG);
			}
			break;
		case PARTICLE_SENDER_NODE:
			if(v_list_count_items(&ctx->verse.particle_scene_node->senders) < (int32)ctx->sender_count) {
				sender_node = create_particle_sender_node(ctx->verse.particle_scene_node, node_id);

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

				/* Is it sender node created by this client */
				if(parent_id == ctx->verse.avatar_id) {
					ctx->sender = sender;
				}

				/* Add node to lookup table*/
				lu_add_item(ctx->verse.lu_table, node_id, sender_node);

				v_list_add_tail(&ctx->verse.particle_scene_node->senders, sender_node);

				vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY,
						node_id, 0, 0);
				vrs_send_node_link(session_id, VRS_DEFAULT_PRIORITY,
						ctx->verse.particle_scene_node->node_id, node_id);
				vrs_send_taggroup_create(session_id, VRS_DEFAULT_PRIORITY,
						node_id, PARTICLE_SENDER_TG);
				vrs_send_layer_create(session_id, VRS_DEFAULT_PRIORITY,
						node_id, 0xFFFF, VRS_VALUE_TYPE_REAL32, 3, PARTICLE_POS_LAYER);
			}
			break;
		}
	}
}

static void cb_receive_node_destroy(const uint8 session_id,
		const uint32 node_id)
{
#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, node_id: %d\n",
			__FUNCTION__, session_id, node_id);
#else
	(void)session_id;
	(void)node_id;
#endif
}

static void cb_receive_connect_accept(const uint8 session_id,
      const uint16 user_id,
      const uint32 avatar_id)
{
#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, user_id: %d, avatar_id: %d\n",
			__FUNCTION__, session_id, user_id, avatar_id);
#endif

	ctx->verse.avatar_id = avatar_id;
	ctx->verse.user_id = user_id;

	/* Subscribe to avatar node */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, avatar_id, 0, 0);

	/* TODO: Create new particle scene node only when there is no other particle scene node */
	vrs_send_node_create(session_id, VRS_DEFAULT_PRIORITY, PARTICLE_SCENE_NODE);

	/* Create new node (particle sender node) */
	vrs_send_node_create(session_id, VRS_DEFAULT_PRIORITY, PARTICLE_SENDER_NODE);
}

static void cb_receive_connect_terminate(const uint8 session_id,
		const uint8 error_num)
{
#if NO_DEBUG_PRINT != 1
	printf("%s() session_id: %d, error_num: %d\n",
			__FUNCTION__, session_id, error_num);
#else
	(void)session_id;
	(void)error_num;
#endif
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

	/* Debug print */
#if NO_DEBUG_PRINT != 1
	printf("%s() username: %s, auth_methods_count: %d, methods: ",
			__FUNCTION__, username, auth_methods_count);
#endif

	for(i=0; i<auth_methods_count; i++) {
		if(methods[i]==VRS_UA_METHOD_PASSWORD)
			is_passwd_supported = 1;
	}

	/* Get username, when it is requested */
	if(username == NULL) {
		if(ctx->verse.username == NULL) {
			printf("Username: ");
			scanf("%s", name);
			attempts = 0;	/* Reset counter of auth. attempt. */
			vrs_send_user_authenticate(session_id, name, VRS_UA_METHOD_NONE, NULL);
		} else {
			vrs_send_user_authenticate(session_id, ctx->verse.username, VRS_UA_METHOD_NONE, NULL);
		}
	} else {
		if(is_passwd_supported == 1) {
			strcpy(name, username);
			/* Print this warning, when previous authentication attempt failed. */
			if(attempts > 0)
				printf("Permission denied, please try again.\n");
			if(ctx->verse.password == 0) {
				/* Get password from user */
				password = getpass("Password: ");
				attempts++;
				vrs_send_user_authenticate(session_id, username, VRS_UA_METHOD_PASSWORD, password);
			} else {
				vrs_send_user_authenticate(session_id, username, VRS_UA_METHOD_PASSWORD, ctx->verse.password);
			}
		} else {
			printf("ERROR: Verse server does not support password authentication method\n");
		}
	}
}

static void register_cb_func_particle_sender(void)
{
	/* Register callback functions */
	vrs_register_receive_user_authenticate(cb_receive_user_authenticate);
	vrs_register_receive_connect_accept(cb_receive_connect_accept);
	vrs_register_receive_connect_terminate(cb_receive_connect_terminate);

	vrs_register_receive_node_create(cb_receive_node_create);
	vrs_register_receive_node_destroy(cb_receive_node_destroy);
	vrs_register_receive_node_link(cb_receive_node_link);

	vrs_register_receive_taggroup_create(cb_receive_taggroup_create);
	vrs_register_receive_taggroup_destroy(cb_receive_taggroup_destroy);
	vrs_register_receive_tag_create(cb_receive_tag_create);
	vrs_register_receive_tag_destroy(cb_receive_tag_destroy);
	vrs_register_receive_tag_set_value(cb_receive_tag_set_value);

	vrs_register_receive_layer_create(cb_receive_layer_create);
	vrs_register_receive_layer_set_value(cb_receive_layer_set_value);
}

/**
 * When receiver set up trigger, then sender sends particle position
 * each frame.
 */
static void verse_send_data(void)
{
	if(ctx->sender == NULL) return;

	pthread_mutex_lock(&ctx->sender->timer->mutex);

	if(ctx->sender->timer->run == 1) {
		uint16 item_id;

		/* Send position for current frame */
		if(ctx->sender->timer->frame >=0 &&
				ctx->sender->timer->frame < ctx->pd->frame_count)
		{

			/* Send current frame */
			vrs_send_tag_set_value(ctx->verse.session_id,
					VRS_DEFAULT_PRIORITY,
					ctx->sender->sender_node->node_id,
					ctx->sender->sender_node->particle_taggroup_id,
					ctx->sender->sender_node->particle_frame_tag_id,
					VRS_VALUE_TYPE_UINT16,
					1,
					&ctx->sender->timer->frame);

			/* For all particles of sender ... */
			for(item_id = 0; item_id < ctx->pd->particle_count; item_id++) {
				/* Send all active particles */
				if(ctx->pd->particles[item_id].states[ctx->sender->timer->frame].state == PARTICLE_STATE_ACTIVE) {
					vrs_send_layer_set_value(ctx->verse.session_id,
							VRS_DEFAULT_PRIORITY,
							ctx->sender->sender_node->node_id,
							ctx->sender->sender_node->particle_layer_id,
							item_id,
							VRS_VALUE_TYPE_REAL32,
							3,
							ctx->pd->particles[item_id].states[ctx->sender->timer->frame].pos);
				}
			}
		}

		/* When animation is at the end, then "delete" particles */
		if(ctx->sender->timer->tot_frame >=0 &&
				ctx->sender->timer->frame == 0)
		{
			/* For all particles of sender ... */
			for(item_id = 0; item_id < ctx->pd->particle_count; item_id++) {
				/* Unset value (delete position) */
				vrs_send_layer_unset_value(ctx->verse.session_id,
						VRS_DEFAULT_PRIORITY,
						ctx->sender->sender_node->node_id,
						ctx->sender->sender_node->particle_layer_id,
						item_id);
			}
		}
	}

	pthread_mutex_unlock(&ctx->sender->timer->mutex);
}

int particle_sender_loop(struct Client_CTX *ctx_)
{
	int ret;

	ctx = ctx_;

	/* Handle SIGINT signal. The handle_signal function will try to terminate
	 * connection. */
	signal(SIGINT, handle_signal);

	register_cb_func_particle_sender();

	if((ret = vrs_send_connect_request(ctx->verse.server_name, "12345",
			VRS_SEC_DATA_NONE ,&ctx->verse.session_id))!=VRS_SUCCESS) {
		printf("ERROR: %s\n", vrs_strerror(ret));
		return 0;
	}

	/* Never ending loop */
	while(1) {
		sem_wait(&ctx->timer_sem);
		vrs_callback_update(ctx->verse.session_id);
		verse_send_data();
	}

	return 1;
}
