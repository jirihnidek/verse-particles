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

#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>

#include <verse.h>

#include "timer.h"
#include "client.h"
#include <stdio.h>

static struct Client_CTX *ctx = NULL;

/**
 * \brief This function creates and initialize new timer structure
 */
struct Timer *create_timer(void)
{
	struct Timer *timer;

	timer = (struct Timer*)malloc(sizeof(struct Timer));

	if(timer != NULL) {
		timer->frame = -1;
		timer->tot_frame = -1;
		timer->run = 0;
		pthread_mutex_init(&timer->mutex, NULL);
	}

	return timer;
}

/**
 * \brief Main timer thread loop
 */
void *timer_loop(void *arg)
{
	struct Particle_Sender *sender;
	struct timeval	current_tv, first_tv, expected_tv;
	unsigned long	sec, usec, counter;
	long 			delay;

	ctx = (struct Client_CTX*)arg;

	counter = 0;
	gettimeofday(&first_tv, NULL);

	while(1) {
		gettimeofday(&current_tv, NULL);

		usec = (counter+1)*(ONE_SECOND/ctx->verse.fps);

		sec = usec / ONE_SECOND;
		usec = usec % ONE_SECOND;

		expected_tv.tv_sec = first_tv.tv_sec + sec;
		expected_tv.tv_usec = first_tv.tv_usec + usec;

		sec = expected_tv.tv_sec - current_tv.tv_sec;
		usec = expected_tv.tv_usec - current_tv.tv_usec;

		delay = ONE_SECOND*sec + usec;

		if(delay>0) {
			counter++;
		} else {
			/* When packing and sending of packet took more then DELAY, then
			 * crop delay and change counter and frame to corresponding
			 * values */
			delay = (-delay)%(ONE_SECOND/ctx->verse.fps);
			counter += 1 + (-delay)/(ONE_SECOND/ctx->verse.fps);
		}

		usleep(delay);

		for(sender = (struct Particle_Sender*)ctx->senders.first;
				sender != NULL;
				sender = sender->next)
		{
			pthread_mutex_lock(&sender->timer->mutex);

			if(sender->timer->run == 1) {
				if(delay>0) {
					sender->timer->tot_frame++;
				} else {
					/* When packing/unpacking and sending/receiving of packet took
					 * more then DELAY, then crop delay and change counter and
					 * frame to corresponding values */
					sender->timer->tot_frame += 1 + (-delay)/(ONE_SECOND/ctx->verse.fps);
				}

				/* Crop frame to be in limit:  <0, frame_count-1> */
				if(sender->timer->tot_frame < 0) {
					sender->timer->frame = 0;
				} else {
					sender->timer->frame = sender->timer->tot_frame % (ctx->pd->frame_count -1);
				}
			}

			pthread_mutex_unlock(&sender->timer->mutex);
		}

		/*printf("frame: %d\n", ctx->timer->tot_frame);*/

		sem_post(&ctx->timer_sem);
	}

	return NULL;
}
