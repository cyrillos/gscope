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
 * cscope backend
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

#include "cscope.h"
#include "backend-proto.h"
#include "util.h"

struct cscope_query_result {
	struct list_head		list;
	void				*data_copy;
	struct backend_proto_item	item;
};

struct cscope_query_param {
	unsigned long			opcode;
	const char			*key;
	const char			*desc;
} cscope_query_params[] = {
	{ CSCOPE_KEY_VERBOSE,		"-v", "Verbose" },
	{ CSCOPE_KEY_CROSSREF,		"-b", "Build the cross-reference only" },
	{ CSCOPE_KEY_NOCASE, 		"-C", "Ignore letter case" },
	{ CSCOPE_KEY_NOCOMP,		"-c", "Do not compress the data" },
	{ CSCOPE_KEY_DRY,		"-d", "Do not update the cross-reference" },
	{ CSCOPE_KEY_KERNEL,		"-k", "Kernel mode" },
	{ CSCOPE_KEY_SINGLE,		"-L", "Single search" },
	{ CSCOPE_KEY_LUI,		"-l", "Line oriented interface" },
	{ CSCOPE_KEY_INVERT,		"-q", "Inverted symbols" },
	{ CSCOPE_KEY_FORCE,		"-u", "Unconditional rebuild" },

	{ CSCOPE_KEY_QRY_REFERENCES,	"-0", "Find References" },
	{ CSCOPE_KEY_QRY_DEFINITION,	"-1", "Find Definition" },
	{ CSCOPE_KEY_QRY_CALLED_FUNC,	"-2", "Find Called Functions" },
	{ CSCOPE_KEY_QRY_CALLING_FUNC,	"-3", "Find Calling Functions" },
	{ CSCOPE_KEY_QRY_TEXT,		"-4", "Find Text" },
	{ CSCOPE_KEY_QRY_EGREP,		"-5", "Find Egrep Pattern" },
	{ CSCOPE_KEY_QRY_FILE,		"-7", "Find File" },
	{ CSCOPE_KEY_QRY_INCLUDING,	"-8", "Find Including" },
};

#define valid_qtype(type) (type > CSCOPE_KEY_MIN && type < CSCOPE_KEY_MAX)
static struct cscope_query_param *xfind_param(unsigned int opcode)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(cscope_query_params); i++) {
		if (cscope_query_params[i].opcode == opcode)
			return &cscope_query_params[i];
	}
	BUG();
	return NULL;
}

const char *cscope_param_desc(unsigned int opcode)
{
	struct cscope_query_param *param = xfind_param(opcode);
	if (param)
		return param->desc;
	return NULL;
}

static struct cscope_query_result *cscope_parse(char *str, unsigned long key)
{
	char *func, *file, *line, *text, *data_copy;
	struct cscope_query_result *r = NULL;

	if (!str || !str[0])
		goto err;

	data_copy = xstrdup(str);

	file = skip_spaces(strtok(data_copy,	" \t"));
	func = skip_spaces(strtok(NULL,		" \t"));
	line = skip_spaces(strtok(NULL,		" \t"));
	text = skip_spaces(strtok(NULL,		"\r\n"));

	if (!func || !file || !line || !text) {
		free(data_copy);
		goto err;
	}

	r = xzalloc(sizeof(*r));

	r->data_copy	= data_copy;
	r->item.key	= key;
	r->item.func	= func;
	r->item.file	= file;
	r->item.line	= atoi(line);
	r->item.text	= text;

	INIT_LIST_HEAD(&r->list);

err:
	return r;
}

static void cscope_query_free(struct list_head *head)
{
	struct cscope_query_result *r, *n;

	list_for_each_entry_safe(r, n, head, list) {
		free(r->data_copy);
		free(r);
	}
}

#define MAX_CMDLINE 256
static const char cscope_cmd[] = "cscope ";

static struct list_head *cscope_query(struct backend_proto_query_params *backend_params)
{
	char cmd_buf[MAX_CMDLINE] = { 0 };
	char read_buf[1024];
	struct list_head *head = NULL;
	struct cscope_query_result *r;
	struct cscope_query_param *qparam;
	unsigned long key = 0;
	FILE *pipe;

	if (!backend_params->u.cscope.name	||
		!*backend_params->u.cscope.name ||
		!valid_qtype(backend_params->u.cscope.type))
		return NULL;

#define strlcat_buf(str) strlcat(cmd_buf, str, sizeof(cmd_buf))
	strlcat_buf(cscope_cmd);

	qparam = xfind_param(backend_params->u.cscope.type);
	strlcat_buf(qparam->key);
	strlcat_buf(backend_params->u.cscope.name);
	strlcat_buf(" ");

	qparam = xfind_param(CSCOPE_KEY_DRY);
	strlcat_buf(qparam->key);
	strlcat_buf(" ");

	qparam = xfind_param(CSCOPE_KEY_SINGLE);
	strlcat_buf(qparam->key);

	if (backend_params->u.cscope.nocase) {
		strlcat_buf(" ");
		strlcat_buf(cscope_query_params[CSCOPE_KEY_NOCASE].key);
		strlcat_buf(" ");
		strlcat_buf(cscope_query_params[CSCOPE_KEY_INVERT].key);
	}
#undef strlcat_buf

	head = xalloc(sizeof(*head));
	INIT_LIST_HEAD(head);

	pipe = popen(cmd_buf, "r");
	if (!pipe) {
		warning("Can't open pipe for : %s", cscope_cmd);
		free(head), head = NULL;
		goto err;
	}

	/*
	 * We assume the output line will not exceed the buffer
	 * otherwise the parsing would not be strictly correct
	 */
	while (fgets(read_buf, sizeof(read_buf), pipe)) {
		r = cscope_parse(read_buf, key++);
		if (r)
			list_add(&r->list, head);
	}
	if (pclose(pipe))
		warning("Closing pipe for parsing '%s' failed",
			backend_params->u.cscope.name);

err:
	return head;
}

static struct cscope_query_result *cscope_lookup_key(struct list_head *head, unsigned long key)
{
	struct cscope_query_result *t;
	list_for_each_entry(t, head, list) {
		if (t->item.key == key)
			return t;
	}
	//debug("Failed for key: %lu", key);
	return NULL;
}

static int cscope_backend_proto_lookup(struct list_head *head, struct backend_proto_item *item, unsigned long key)
{
	struct cscope_query_result *t = cscope_lookup_key(head, key);
	if (!t)
		return -1;
	memcpy(item, &t->item, sizeof(*item));
	return 0;
}

static int cscope_backend_proto_get(struct list_head *entry, struct backend_proto_item *item)
{
	struct cscope_query_result *t = container_of(entry, struct cscope_query_result, list);
	memcpy(item, &t->item, sizeof(*item));
	return 0;
}

static void cscope_backend_proto_destory(struct list_head *head)
{
	struct cscope_query_result *t = container_of(head, struct cscope_query_result, list);
	cscope_query_free(&t->list);
	free(t);
}

static void cscope_backend_proto_refresh(void)
{
	struct cscope_query_param *qparam;
	char cmd_buf[MAX_CMDLINE] = { 0 };
	FILE *pipe;

	strlcat(cmd_buf, cscope_cmd, sizeof(cmd_buf));

	qparam = xfind_param(CSCOPE_KEY_CROSSREF);
	strlcat(cmd_buf, (char *)qparam->key, sizeof(cmd_buf));
	strlcat(cmd_buf, (char *)" ", sizeof(cmd_buf));

	qparam = xfind_param(CSCOPE_KEY_FORCE);
	strlcat(cmd_buf, (char *)qparam->key, sizeof(cmd_buf));
	strlcat(cmd_buf, (char *)" ", sizeof(cmd_buf));

	qparam = xfind_param(CSCOPE_KEY_KERNEL);
	strlcat(cmd_buf, (char *)qparam->key, sizeof(cmd_buf));
	strlcat(cmd_buf, (char *)" ", sizeof(cmd_buf));

	pipe = popen(cmd_buf, "r");
	if (!pipe) {
		warning("Can't open pipe for : %s", cscope_cmd);
		return;
	}

	if (pclose(pipe))
		warning("Closing pipe for parsing '%s' failed",
			cmd_buf);
}

void init_cscope_backend_proto(void)
{
	static struct backend_proto proto = {
		.protocol_name			= "cscope",
		.protocol			= BACKEND_PROTO_CSCOPE,
		.backend_proto_query		= cscope_query,
		.backend_proto_get		= cscope_backend_proto_get,
		.backend_proto_lookup		= cscope_backend_proto_lookup,
		.backend_proto_destory		= cscope_backend_proto_destory,
		.backend_proto_refresh		= cscope_backend_proto_refresh,
	};

	backend_proto_register(&proto);
}
