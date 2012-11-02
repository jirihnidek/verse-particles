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
			vrs_send_connect_terminate(ctx->verse.session_id);
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
	struct ParticleNode *particle_node;
	struct ParticleSenderNode *sender_node;

	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, vec: (%6.3f, %6.3f, %6.3f)\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, vec[0], vec[1], vec[2]);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_NODE:
			particle_node = (struct ParticleNode*)node;
			sender_node = particle_node->sender;

			if(particle_node->particle_taggroup_id == taggroup_id) {
				if(particle_node->pos_tag_id == tag_id) {
					struct RefParticleState *ref_state;
					uint16 current_frame;

					pthread_mutex_lock(&ctx->timer->mutex);
					current_frame = ctx->timer->frame;
					pthread_mutex_unlock(&ctx->timer->mutex);

					if(particle_node->ref_particle != NULL &&
							particle_node->rec_particle != NULL)
					{
						/* Find reference state */
						ref_state = find_ref_particle_state(ctx->pd,
								particle_node->ref_particle,
								sender_node->sender->rec_pd->rec_frame,
								vec);

						if(ref_state != NULL) {
							struct ReceivedParticleState *rec_state;

							pthread_mutex_lock(&sender_node->sender->rec_pd->mutex);

							rec_state = &particle_node->rec_particle->received_states[ref_state->frame];

							/* Set up first, last and current received state */
							if(particle_node->rec_particle->first_received_state == NULL) {
								particle_node->rec_particle->first_received_state = rec_state;
								particle_node->rec_particle->last_received_state = rec_state;
							} else {
								if(particle_node->rec_particle->first_received_state->ref_particle_state->frame > rec_state->ref_particle_state->frame) {
									particle_node->rec_particle->first_received_state = rec_state;
								}
								if(particle_node->rec_particle->last_received_state->ref_particle_state->frame < rec_state->ref_particle_state->frame) {
									particle_node->rec_particle->last_received_state = rec_state;
								}
							}

							/* This state is the current received */
							particle_node->rec_particle->current_received_state = rec_state;

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

							pthread_mutex_unlock(&sender_node->sender->rec_pd->mutex);
						} else {
							printf("ERROR: Reference particle state not found\n");
						}
					} else {
						printf("ERROR: Reference particle state not found\n");
					}

				}
			}
			break;
		}
	} else {
		printf("ERROR: node not found\n");
	}
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
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;

	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, data_type: %d, count: %d, value: %p\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, data_type, count, value);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(scene_node->particle_taggroup_id == taggroup_id) {
				if(scene_node->particle_frame_tag_id == tag_id) {

					if(value < 0) {
						pthread_mutex_lock(&ctx->timer->mutex);
						if(ctx->timer->run == 0) {
							ctx->timer->run = 1;
							ctx->timer->tot_frame = value;
						}
						pthread_mutex_unlock(&ctx->timer->mutex);
					} else {
						/* TODO: use this for synchronization of time */
					}

					scene_node->received_frame = value;

					sender_node = scene_node->senders.first;
					while(sender_node != NULL) {
						pthread_mutex_lock(&sender_node->sender->rec_pd->mutex);
						sender_node->sender->rec_pd->rec_frame = value;
						pthread_mutex_unlock(&sender_node->sender->rec_pd->mutex);
						sender_node = sender_node->next;
					}
				}
			}
			break;
		}
	} else {
		printf("ERROR: node not found\n");
	}
}

static void cb_receive_tag_set_uint16(const uint8 session_id,
		const uint32 node_id,
		const uint16 taggroup_id,
		const uint16 tag_id,
		const uint16 value)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;
	struct ParticleNode *particle_node;

	printf("%s() session_id: %u, node_id: %u, taggroup_id: %u, tag_id: %u, value: %u\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, value);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;
			if(scene_node->particle_taggroup_id == taggroup_id) {
				/* TODO: add something useful here */
			}
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;
			if(sender_node->particle_taggroup_id == taggroup_id) {
				/* TODO: add something useful here */
			}
			break;
		case PARTICLE_NODE:
			particle_node = (struct ParticleNode*)node;
			sender_node = particle_node->sender;

			if(particle_node->particle_taggroup_id == taggroup_id) {
				if(particle_node->particle_id_tag_id == tag_id) {
					if(value < ctx->pd->particle_count) {
						particle_node->ref_particle = &ctx->pd->particles[value];
						particle_node->rec_particle = &sender_node->sender->rec_pd->received_particles[value];

						if(value == (ctx->pd->particle_count-1)) {
							printf(">>>Starting animation<<<<\n");
							/* Send frame, when receiver received all needed data */
							vrs_send_tag_set_int16(session_id,
									VRS_VRS_DEFAULT_PRIORITY,
									ctx->verse.particle_scene_node->node_id,
									ctx->verse.particle_scene_node->particle_taggroup_id,
									ctx->verse.particle_scene_node->particle_frame_tag_id,
									-25);
						}

					} else {
						printf("ERROR: Bad particle ID: %d\n", value);
					}
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
		char *custom_type)
{
	struct Node *node;
	struct ParticleSceneNode *scene_node;
	struct ParticleSenderNode *sender_node;
	struct ParticleNode *particle_node;

	printf("%s() session_id: %d, node_id: %d, taggroup_id: %d, tag_id: %d, type: %d, name: %s\n",
				__FUNCTION__, session_id, node_id, taggroup_id, tag_id, data_type, custom_type);

	node = lu_find(ctx->verse.lu_table, node_id);

	if(node != NULL) {
		switch(node->type) {
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;

			if(scene_node->particle_taggroup_id == taggroup_id) {
				if(data_type == VRS_VALUE_TYPE_UINT16 && custom_type == PARTICLE_FRAME) {
					scene_node->particle_frame_tag_id = tag_id;
				} else if(data_type == VRS_VALUE_TYPE_UINT16 && custom_type == SENDER_COUNT) {
					scene_node->sender_count_tag_id = tag_id;
				}
			}
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(sender_node->particle_taggroup_id == taggroup_id) {
				if(data_type == VRS_VALUE_TYPE_REAL32 && count == 3, custom_type == POSITION) {
					sender_node->pos_tag_id = tag_id;
				} else if(data_type == VRS_VALUE_TYPE_UINT16 && custom_type == PARTICLE_COUNT) {
					sender_node->count_tag_id = tag_id;
				} else if(data_type == TAG_TYPE_UINT16 && strcmp(custom_type, "ID") == 0) {
					sender_node->sender_id_tag_id = tag_id;
				}
			}
			break;
		case PARTICLE_NODE:
			particle_node = (struct ParticleNode*)node;

			if(particle_node->particle_taggroup_id == taggroup_id) {
				if(data_type == TAG_TYPE_VEC3_REAL32 && strcmp(custom_type, "Position") == 0) {
					particle_node->pos_tag_id = tag_id;
				} else if(data_type == TAG_TYPE_UINT16 && strcmp(custom_type, "ID") == 0) {
					particle_node->particle_id_tag_id = tag_id;
				}
			}
			break;
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
		case PARTICLE_SCENE_NODE:
			scene_node = (struct ParticleSceneNode *)node;
			if(scene_node->node_id == node_id &&
					strcmp(name, "Particle_Scene") == 0)
			{
				scene_node->particle_taggroup_id = taggroup_id;
				/* Subscribe to this tag group */
				vrs_send_taggroup_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, taggroup_id, 0, 0);
			}
			break;
		case PARTICLE_SENDER_NODE:
			sender_node = (struct ParticleSenderNode*)node;

			if(strcmp(name, "Particle_Sender") == 0) {
				sender_node->particle_taggroup_id = taggroup_id;
				/* Subscribe to this tag group */
				vrs_send_taggroup_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, taggroup_id, 0, 0);
			}
			break;
		case PARTICLE_NODE:
			particle_node = (struct ParticleNode*)node;
			if(strcmp(name, "Particle") == 0) {
				particle_node->particle_taggroup_id = taggroup_id;
				/* Subscribe to this tag group */
				vrs_send_taggroup_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, taggroup_id, 0, 0);
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
		const uint16 user_id)
{
	printf("%s() session_id: %d, node_id: %d, parent_id: %d, user_id: %d\n",
			__FUNCTION__, session_id, node_id, parent_id, user_id);

	if(user_id != ctx->verse.user_id) {
		return;
	}

	if(ctx->verse.particle_scene_node == NULL && parent_id == VRS_SCENE_PARENT_NODE_ID) {
		/* Create node of particle scene */
		ctx->verse.particle_scene_node = create_particle_scene_node(node_id);

		/* Add node to lookup table*/
		lu_add_item(ctx->verse.lu_table, node_id, ctx->verse.particle_scene_node);

		/* Subscribe to this node */
		vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, 0);

	} else if(ctx->verse.particle_scene_node != NULL) {
		if(parent_id == ctx->verse.particle_scene_node->node_id) {
			if(v_list_count_items(&ctx->verse.particle_scene_node->senders) < (int32)ctx->sender_count) {
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
					sender_node->sender = sender;
					sender->sender_node = sender_node;
				}

				/* Add node to lookup table*/
				lu_add_item(ctx->verse.lu_table, node_id, sender_node);

				v_list_add_tail(&ctx->verse.particle_scene_node->senders, sender_node);

				vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, 0);
			}
		} else {
			struct Node *node = lu_find(ctx->verse.lu_table, parent_id);

			if(node!=NULL) {
				struct ParticleSenderNode *sender_node = (struct ParticleSenderNode*)node;
				if(v_list_count_items(&sender_node->particles) < ctx->pd->particle_count) {
					struct ParticleNode *particle_node = create_particle_node(sender_node, node_id);

					/* Add node to lookup table*/
					lu_add_item(ctx->verse.lu_table, node_id, particle_node);

					v_list_add_tail(&sender_node->particles, particle_node);

					vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, node_id, 0);
				}
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

static void cb_receive_node_link(const uint8 session_id,
		const uint32 parent_node_id,
		const uint32 child_node_id)
{
	printf("%s() session_id: %d, parent_node_id: %d, child_node_id: %d\n",
			__FUNCTION__, session_id, parent_node_id, child_node_id);
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
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, avatar_id, 0);

	/* Subscribe to parent of scenes */
	vrs_send_node_subscribe(session_id, VRS_DEFAULT_PRIORITY, VRS_SCENE_PARENT_NODE_ID, 0);

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

	vrs_register_receive_tag_set_value(cb_receive_tag_set_uint16);
	vrs_register_receive_tag_set_int16(cb_receive_tag_set_int16);

	vrs_register_receive_tag_set_vec3_real32(cb_receive_tag_set_vec3_real32);
}

void *particle_receiver_loop(void *arg)
{
	int ret;

	ctx = (struct Client_CTX*)arg;

	/* Handle SIGINT signal. The handle_signal function will try to terminate
	 * connection. */
	signal(SIGINT, handle_signal);

	register_cb_func_particle_receiver();

	if((ret = vrs_send_connect_request(ctx->verse.server_name, "12345", DGRAM_SEC_NONE ,&ctx->verse.session_id))!=VC_SUCCESS) {
		printf("ERROR: %s\n", verse_strerror(ret));
		return 0;
	}

	/* Never ending loop */
	while(1) {
		/*usleep(1000000/ctx->verse.fps);*/
		sem_wait(&ctx->timer_sem);
		verse_callback_update(ctx->verse.session_id);
	}

	return NULL;
}
