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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <verse.h>

#include "client.h"
#include "client_particle_sender.h"
#include "client_particle_receiver.h"
#include "particle_data.h"
#include "display_glut.h"
#include "lu_table.h"
#include "timer.h"
#include "sender.h"


/**
 * \brief Clean client context
 */
static void clean_client_ctx(struct Client_CTX *ctx)
{
	if(ctx->pd != NULL) {
		/* Free reference particle data */
		free_ref_particle_data(ctx->pd);
		free(ctx->pd);
		ctx->pd = NULL;
	}

	if(ctx->verse.particle_scene_node != NULL) {
		free(ctx->verse.particle_scene_node);	/* TODO: create and use destructor */
		ctx->verse.particle_scene_node = NULL;
	}

	if(ctx->verse.server_name != NULL) {
		free(ctx->verse.server_name);
		ctx->verse.server_name = NULL;
	}

	if(ctx->verse.lu_table != NULL) {
		lu_table_free(ctx->verse.lu_table);
		ctx->verse.lu_table = NULL;
	}

	if(ctx->verse.username != NULL) {
		free(ctx->verse.username);
		ctx->verse.username = NULL;
	}

	if(ctx->verse.password != NULL) {
		free(ctx->verse.password);
		ctx->verse.password = NULL;
	}

	if(ctx->sender != NULL && ctx->sender->timer != NULL) {
		pthread_mutex_destroy(&ctx->sender->timer->mutex);
		free(ctx->sender->timer);
		ctx->sender->timer = NULL;
	}

	if(ctx->senders.first != NULL) {
		struct Particle_Sender *sender;

		sender = ctx->senders.first;
		while(sender != NULL) {
			if(sender->rec_pd != NULL) {
				free_received_particle_data(sender->rec_pd);
				free(sender->rec_pd);
				sender->rec_pd = NULL;
			}
			sender = sender->next;
		}

		v_list_free(&ctx->senders);
	}

	ctx->verse.session_id = -1;
}


/**
 * \brief Set default values of client context
 */
static void init_client_ctx(struct Client_CTX *ctx)
{
	ctx->flags = 0;
	ctx->senders.first = ctx->senders.last = NULL;
	ctx->sender_count = DEFAULT_SENDER_COUNT;
	ctx->client_type = CLIENT_NONE;
	ctx->pd = NULL;
	ctx->display = create_particle_display();
	ctx->verse.fps = DEFAULT_FPS;
	ctx->verse.particle_scene_node = NULL;
	ctx->verse.server_name = NULL;
	ctx->verse.session_id = -1;
	ctx->verse.lu_table = NULL;
	ctx->verse.username = NULL;
	ctx->verse.password = NULL;
	ctx->receiver_thread = 0;
	ctx->timer_thread = 0;
	ctx->sender = NULL;
	sem_init(&ctx->timer_sem, 0, 0);
}


/**
 * \brief Set type of client
 */
static int set_client_type(struct Client_CTX *ctx, char *c_type)
{
	int ret = 0;

	if(strcmp(c_type, "receiver")==0) {
		ctx->client_type = CLIENT_RECEIVER;
		ret = 1;
	} else if(strcmp(c_type, "sender")==0) {
		ctx->client_type = CLIENT_SENDER;
		ret = 1;
	} else {
		printf("ERROR: Unsupported client type: %s\n", c_type);
	}

	return ret;
}


/**
 * \brief Set type of particle visualization
 */
static int set_visual_type(struct Client_CTX *ctx, char *v_type)
{
	int ret = 0;

	if(strcmp(v_type, "lines")==0) {
		ctx->display->visual_type = VISUAL_LINE;
		ret = 1;
	} else if(strcmp(v_type, "dots")==0) {
		ctx->display->visual_type = VISUAL_DOT;
		ret = 1;
	} else if(strcmp(v_type, "dot-lines")==0) {
		ctx->display->visual_type = VISUAL_DOT_LINE;
		ret = 1;
	} else if(strcmp(v_type, "none")==0) {
		ctx->display->visual_type = VISUAL_SIMPLE;
		ret = 1;
	} else {
		printf("ERROR: Unsupported visual type: %s\n", v_type);
	}

	return ret;
}


/**
 * \brief Set type of client debug level
 */
static int set_debug_level(char *debug_level)
{
	int ret = VRS_FAILURE;

	if( strcmp(debug_level, "debug") == 0) {
		ret = vrs_set_debug_level(VRS_PRINT_DEBUG_MSG);
	} else if( strcmp(debug_level, "warning") == 0 ) {
		ret = vrs_set_debug_level(VRS_PRINT_WARNING);
	} else if( strcmp(debug_level, "error") == 0 ) {
		ret = vrs_set_debug_level(VRS_PRINT_ERROR);
	} else if( strcmp(debug_level, "info") == 0 ) {
		ret = vrs_set_debug_level(VRS_PRINT_INFO);
	} else if( strcmp(debug_level, "none") == 0 ) {
		ret = vrs_set_debug_level(VRS_PRINT_NONE);
	} else {
		printf("ERROR: Unsupported debug level: %s\n", debug_level);
	}

	return (ret==VRS_SUCCESS)?1:0;
}


/**
 * \brief Print help
 */
static void print_help(char *prog_name)
{
	printf("\n Usage: %s [OPTION...] -t client_type server_address particle_directory\n", prog_name);
	printf("\n");
	printf("  This program is Verse client receiving/sending particles\n");
	printf("  from/to Verse server.\n");
	printf("\n");
	printf("  Options:\n");
	printf("   -t client_type   type of client: [sender|receiver]\n");
	printf("   -v visual_type   use visual type [none|lines|dots|dot-lines]\n");
	printf("                      (default: none)\n");
	printf("   -d debug_level   use debug level [none|info|error|warning|debug]\n");
	printf("                      (default: debug)\n");
	printf("   -f fps           use defined FPS value (default value is 25)\n");
	printf("   -h               display this help and exit\n");
	printf("   -s               secure UDP connection with DTLS protocol\n");
	printf("   -c               make screen-cast to TGA files\n");
	printf("   -u username      username used for authentication\n");
	printf("   -p password      password used for authentication\n");
	printf("\n");
}


int main(int argc, char *argv[])
{
	struct Client_CTX ctx;
	int opt, ret;

    init_client_ctx(&ctx);

	/* When client was started with some arguments */
	if(argc > 1) {
		/* Parse all options */
		while( (opt = getopt(argc, argv, "shcv:d:t:f:n:u:p:")) != -1) {
			switch(opt) {
				case 's':
					ctx.flags |= VC_DGRAM_SEC_DTLS;
					break;
				case 'v':
					ret = set_visual_type(&ctx, optarg);
					if(ret != 1) {
						print_help(argv[0]);
						clean_client_ctx(&ctx);
						exit(EXIT_FAILURE);
					}
					break;
				case 'c':
					ctx.flags |= VC_MAKE_SCREENCAST;
					break;
				case 'd':
					ret = set_debug_level(optarg);
					if(ret != 1) {
						print_help(argv[0]);
						clean_client_ctx(&ctx);
						exit(EXIT_FAILURE);
					}
					break;
				case 't':
					ret = set_client_type(&ctx, optarg);
					if(ret != 1) {
						print_help(argv[0]);
						clean_client_ctx(&ctx);
						exit(EXIT_FAILURE);
					}
					break;
				case 'f':
					if(sscanf(optarg, "%u", &ctx.verse.fps) != 1) {
						ctx.verse.fps = DEFAULT_FPS;
					}
					break;
				case 'u':
					ctx.verse.username = strdup(optarg);
					break;
				case 'p':
					ctx.verse.password = strdup(optarg);
					break;
				case 'h':
					print_help(argv[0]);
					clean_client_ctx(&ctx);
					exit(EXIT_SUCCESS);
				case ':':
					clean_client_ctx(&ctx);
					exit(EXIT_FAILURE);
				case '?':
					clean_client_ctx(&ctx);
					exit(EXIT_FAILURE);
			}
		}

		/* The last two arguments have to be name of server and name of
		 * directory containing particles */
		if(optind+2 != argc) {
			printf("ERROR: Bad number of parameters: %d != 2\n", argc - optind);
			print_help(argv[0]);
			clean_client_ctx(&ctx);
			return EXIT_FAILURE;
		}
	} else {
		printf("ERROR: Minimal number of arguments: 2\n");
		print_help(argv[0]);
		clean_client_ctx(&ctx);
		return EXIT_FAILURE;
	}

	if(ctx.client_type == CLIENT_NONE) {
		printf("ERROR: No type of client specified\n");
		print_help(argv[0]);
		clean_client_ctx(&ctx);
		return EXIT_FAILURE;
	}

	/* Set up server name */
	ctx.verse.server_name = strdup(argv[optind]);

	/* TODO: Load reference particle data only for -t sender, -t receiver should
	 * read reference data after negotiation with server */
	ctx.pd = read_ref_particle_data(argv[optind+1]);

	/* Create linked list of senders */
	create_senders(&ctx);

	/* Set up node lookup table */
	ctx.verse.lu_table = lu_table_create(10000);	/* TODO: it has to be 10^n and less then max number of particles */

	if(ctx.pd != NULL) {
		switch(ctx.client_type) {
		case CLIENT_NONE:
			return EXIT_FAILURE;
		case CLIENT_SENDER:
			if( pthread_create(&ctx.timer_thread, NULL, timer_loop, (void*)&ctx) == 0) {
				particle_sender_loop(&ctx);
			} else {
				clean_client_ctx(&ctx);
				return EXIT_FAILURE;
			}
			break;
		case CLIENT_RECEIVER:
			if( pthread_create(&ctx.timer_thread, NULL, timer_loop, (void*)&ctx) != 0) {
				clean_client_ctx(&ctx);
				return EXIT_FAILURE;
			} else {
				if( pthread_create(&ctx.receiver_thread, NULL, particle_receiver_loop, (void*)&ctx) == 0) {
					particle_display_loop(&ctx, argc, argv);
				} else {
					clean_client_ctx(&ctx);
					return EXIT_FAILURE;
				}
			}
			break;
		}
	} else {
		clean_client_ctx(&ctx);
		return EXIT_FAILURE;
	}

	/* Free client context */
	clean_client_ctx(&ctx);

	return EXIT_SUCCESS;
}
