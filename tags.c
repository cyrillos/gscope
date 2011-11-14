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
 * ctags backend
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

#include "tags.h"
#include "util.h"

#include "backend-proto.h"

static const struct {
	char opcode;
	int type;
} const clang_tag_codes[] = {
	{ 'c', TAG_CLASS	},
	{ 'd', TAG_MACRO	},
	{ 'e', TAG_ENUMERATOR	},
	{ 'f', TAG_FUNCTION	},
	{ 'g', TAG_ENUM		},
	{ 'l', TAG_LOCAL	},
	{ 'm', TAG_MEMBER	},
	{ 'n', TAG_NAMESPACE	},
	{ 'p', TAG_PROTOTYPE	},
	{ 's', TAG_STRUCTURE	},
	{ 't', TAG_TYPEDEF	},
	{ 'u', TAG_UNION	},
	{ 'v', TAG_VARIABLE	},
	{ 'x', TAG_EXTERNVAR	},
};

static const char * const clang_tag_symname[TAG_MAX] = {
	[TAG_NONE]		= "None",
	[TAG_CLASS]		= "Class",
	[TAG_MACRO]		= "Macro",
	[TAG_ENUMERATOR]	= "Enumerator",
	[TAG_FUNCTION]		= "Function",
	[TAG_ENUM]		= "Enum",
	[TAG_LOCAL]		= "Local",
	[TAG_MEMBER]		= "Member",
	[TAG_NAMESPACE]		= "Namespace",
	[TAG_PROTOTYPE]		= "Prototype",
	[TAG_STRUCTURE]		= "Structure",
	[TAG_TYPEDEF]		= "Typedef",
	[TAG_UNION]		= "Union",
	[TAG_VARIABLE]		= "Variable",
	[TAG_EXTERNVAR]		= "External",

};

static inline const char * const tag_symname_lookup(int type)
{
	if (type < TAG_NONE || type >= TAG_MAX)
		return NULL;

	return clang_tag_symname[type];
}

static int tag_type_lookup(char *str)
{
	int i, ret = TAG_NONE;

	if (!str || !*str)
		return TAG_NONE;

	for (i = 0; i < (int)ARRAY_SIZE(clang_tag_codes); i++)
		if (str[0] == clang_tag_codes[i].opcode)
			return clang_tag_codes[i].type;

	return ret;
}

void tag_dump(struct tag *t)
{
	if (t)
		debug("TAG: %lu:%s:%s:%s:%lu",
			t->key,
			tag_symname_lookup(t->type),
			t->name, t->file, t->line);
	else
		debug("TAG: nil");
}

static struct tag *tag_parse(char *str, unsigned long key)
{
	char *name, *file, *line, *type, *data_copy;
	struct tag *tag = NULL;

	if (!str || !str[0])
		goto err;

	data_copy = xstrdup(str);

	name = strtok(data_copy,"\t");
	file = strtok(NULL,	"\t");
	line = strtok(NULL,	"\t");
	type = strtok(NULL,	"\t\r\n");

	if (!name || !file || !line || !type) {
		free(data_copy);
		goto err;
	}

	tag = xzalloc(sizeof(*tag));

	tag->data_copy	= data_copy;
	tag->name	= name;
	tag->file	= file;
	tag->line	= atoi(line);
	tag->type	= tag_type_lookup(type);
	tag->symtype	= tag_symname_lookup(tag->type);
	tag->key	= key;

	INIT_LIST_HEAD(&tag->list);

err:
	return tag;
}

struct tag *tag_name_lookup(struct list_head *head, char *str)
{
	struct tag *t;

	if (!str || !*str)
		return NULL;

	list_for_each_entry(t, head, list)
		if (!strcmp(t->name, str))
			return t;
	return NULL;
}

struct tag *tag_line_lookup(struct list_head *head, unsigned long line)
{
	struct tag *t;

	list_for_each_entry(t, head, list)
		if (line == t->line)
			return t;
	return NULL;
}

struct tag *tag_key_lookup(struct list_head *head, unsigned long key)
{
	struct tag *t;

	list_for_each_entry(t, head, list)
		if (key == t->key)
			return t;
	return NULL;
}

#define MAX_CMDLINE 256

const char ctags_cmd[] = "ctags --excmd=n -u -f - ";

void tags_free(struct list_head *head)
{
	struct tag *t, *n;

	if (!head)
		return;

	list_for_each_entry_safe(t, n, head, list) {
		free(t->data_copy);
		free(t);
	}
	free(head);
}

struct list_head *tag_parse_file(char *name)
{
	char cmdline[MAX_CMDLINE] = {0};
	char buf[MAX_CMDLINE] = {0};
	struct list_head *head;
	unsigned long key = 0;
	FILE *pipe;

	head = xalloc(sizeof(*head));
	INIT_LIST_HEAD(head);

	strlcat(cmdline, ctags_cmd, sizeof(cmdline));
	strlcat(cmdline, name, sizeof(cmdline));

	pipe = popen(cmdline, "r");
	if (!pipe) {
		warning("Can't open pipe for parsing: %s", name);
		free(head), head = NULL;
		goto err;
	}
	while (fgets(buf, sizeof(buf), pipe)) {
		struct tag *tag = tag_parse(buf, key++);
		if (tag)
			list_add(&tag->list, head);
	}
	if (pclose(pipe))
		warning("Closing pipe for parsing %s failed", name);

err:
	return head;
}

