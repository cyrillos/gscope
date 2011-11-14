/*
 * (c) 2010 Cyrill Gorcunov, gorcunov@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * backend protocol
 */

#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>

#include <pthread.h>

#include "backend-proto.h"
#include "util.h"

static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
static struct backend_proto protocols[BACKEND_PROTO_MAX];

struct backend_proto *backend_proto_register(struct backend_proto *proto)
{
	struct backend_proto *p = NULL;
	unsigned int i, j = -1U;

	pthread_rwlock_rdlock(&rwlock);

	for (i = 0; i < ARRAY_SIZE(protocols); i++) {
		if (protocols[i].protocol == BACKEND_PROTO_NONE) {
			if (j == -1U)
				j = i;
		} else if (protocols[i].protocol == proto->protocol) {
			p = &protocols[i];
			goto unlock;
		}
	}
	if (j != -1U) {
		protocols[j] = *proto;
		p = &protocols[i];
		goto unlock;
	}

unlock:
	pthread_rwlock_unlock(&rwlock);
	return p;
}

void backend_proto_unregister(struct backend_proto *proto)
{
	unsigned int i;

	pthread_rwlock_wrlock(&rwlock);

	for (i = 0; i < ARRAY_SIZE(protocols); i++) {
		if (protocols[i].protocol == proto->protocol) {
			protocols[i].protocol = BACKEND_PROTO_NONE;
			break;
		}
	}

	pthread_rwlock_unlock(&rwlock);
}

struct backend_proto *backend_proto_find(enum backend_protocols proto)
{
	struct backend_proto *p = NULL;
	unsigned int i;

	pthread_rwlock_rdlock(&rwlock);

	for (i = 0; i < ARRAY_SIZE(protocols); i++) {
		if (protocols[i].protocol == proto) {
			p = &protocols[i];
			break;
		}
	}

	pthread_rwlock_unlock(&rwlock);
	return p;
}
