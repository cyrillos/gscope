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
 */

#ifndef TAGS_H
#define TAGS_H

#include "list.h"
#include "backend-proto.h"

enum tag_type {
	TAG_NONE,

	TAG_CLASS,
	TAG_MACRO,
	TAG_ENUMERATOR,
	TAG_FUNCTION,
	TAG_ENUM,
	TAG_LOCAL,
	TAG_MEMBER,
	TAG_NAMESPACE,
	TAG_PROTOTYPE,
	TAG_STRUCTURE,
	TAG_TYPEDEF,
	TAG_UNION,
	TAG_VARIABLE,
	TAG_EXTERNVAR,

	TAG_MAX
};

struct tag {
	struct list_head	list;
	void			*data_copy;
	unsigned long		key;

	const char		*symtype;
	char			*name;
	char			*file;
	unsigned long		line;
	int			type;
};

void tag_dump(struct tag *t);
struct tag *tag_name_lookup(struct list_head *head, char *str);
struct tag *tag_line_lookup(struct list_head *head, unsigned long line);
struct tag *tag_key_lookup(struct list_head *head, unsigned long key);
void tags_free(struct list_head *head);
struct list_head *tag_parse_file(char *name);

#endif/* TAGS_H */
