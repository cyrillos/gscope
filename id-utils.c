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
 * iu-utils backend
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

#include "id-utils.h"
#include "backend-proto.h"
#include "util.h"

struct idutils_query_result {
	struct list_head		list;
	void				*data_copy;
	struct backend_proto_item	item;
};

static struct idutils_query_result *idutils_parse(char *name, char *str, unsigned long key)
{
	char *func, *file, *line, *text, *data_copy;
	struct idutils_query_result *r = NULL;

	if (!str || !str[0])
		goto err;

	data_copy = xstrdup(str);

	func = skip_spaces(name);
	file = skip_spaces(strtok(data_copy,	":"));
	line = skip_spaces(strtok(NULL,		":"));
	text = skip_spaces(strtok(NULL,		"\r\n"));

	if (!func || !file || !line || !text) {
		free(data_copy);
		goto err;
	}

	r = xzalloc(sizeof(*r));

	r->data_copy	= data_copy;
	r->item.key	= key;
	r->item.func	= xstrdup(func);
	r->item.file	= file;
	r->item.line	= atoi(line);
	r->item.text	= text;

	INIT_LIST_HEAD(&r->list);

err:
	return r;
}

static void idutils_query_free(struct list_head *head)
{
	struct idutils_query_result *r, *n;

	list_for_each_entry_safe(r, n, head, list) {
		free(r->item.func);
		free(r->data_copy);
		free(r);
	}
}

#define MAX_CMDLINE 256
static const char idutils_cmd[] = "gid ";
static const char rebuild_cmd[] = "mkid ";

static struct list_head *idutils_query(struct backend_proto_query_params *backend_params)
{
	char cmd_buf[MAX_CMDLINE] = { 0 };
	char read_buf[1024];
	struct idutils_query_result *r;
	struct list_head *head = NULL;
	unsigned long key = 0;
	FILE *pipe;

	if (!backend_params->u.idutils.name	||
		!*backend_params->u.idutils.name)
		return NULL;

#define strlcat_buf(str) strlcat(cmd_buf, str, sizeof(cmd_buf))
	strlcat_buf(idutils_cmd);
	strlcat_buf(backend_params->u.idutils.name);
#undef strlcat_buf

	head = xalloc(sizeof(*head));
	INIT_LIST_HEAD(head);

	pipe = popen(cmd_buf, "r");
	if (!pipe) {
		warning("Can't open pipe for : %s", idutils_cmd);
		free(head), head = NULL;
		goto err;
	}

	/*
	 * We assume the output line will not exceed the buffer
	 * otherwise the parsing would not be strictly correct
	 */
	while (fgets(read_buf, sizeof(read_buf), pipe)) {
		r = idutils_parse(backend_params->u.idutils.name, read_buf, key++);
		if (r)
			list_add(&r->list, head);
	}
	if (pclose(pipe))
		warning("Closing pipe for parsing '%s' failed",
			backend_params->u.idutils.name);

err:
	return head;
}

static struct idutils_query_result *idutils_lookup_key(struct list_head *head, unsigned long key)
{
	struct idutils_query_result *t;
	list_for_each_entry(t, head, list) {
		if (t->item.key == key)
			return t;
	}
	return NULL;
}

static int idutils_backend_proto_lookup(struct list_head *head, struct backend_proto_item *item, unsigned long key)
{
	struct idutils_query_result *t = idutils_lookup_key(head, key);
	if (!t)
		return -1;
	memcpy(item, &t->item, sizeof(*item));
	return 0;
}

static int idutils_backend_proto_get(struct list_head *entry, struct backend_proto_item *item)
{
	struct idutils_query_result *t = container_of(entry, struct idutils_query_result, list);
	memcpy(item, &t->item, sizeof(*item));
	return 0;
}

static void idutils_backend_proto_destory(struct list_head *entry)
{
	struct idutils_query_result *t = container_of(entry, struct idutils_query_result, list);
	idutils_query_free(&t->list);
	free(t);
}

static void idutils_backend_proto_refresh(void)
{
	FILE *pipe;

	pipe = popen(rebuild_cmd, "r");
	if (!pipe) {
		warning("Can't open pipe for : %s", rebuild_cmd);
		return;
	}

	if (pclose(pipe))
		warning("Closing pipe for parsing '%s' failed",
			rebuild_cmd);
}

void init_id_utils_backend_proto(void)
{
	static struct backend_proto proto = {
		.protocol_name		= "id-utils",
		.protocol		= BACKEND_PROTO_ID_UTILS,
		.backend_proto_query	= idutils_query,
		.backend_proto_get	= idutils_backend_proto_get,
		.backend_proto_lookup	= idutils_backend_proto_lookup,
		.backend_proto_destory	= idutils_backend_proto_destory,
		.backend_proto_refresh	= idutils_backend_proto_refresh,
	};

	backend_proto_register(&proto);
}
