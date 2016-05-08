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

#ifndef BACKEND_PROTO_H
#define BACKEND_PROTO_H

#include "list.h"

enum backend_protocols {
	BACKEND_PROTO_NONE,

	BACKEND_PROTO_CSCOPE,
	BACKEND_PROTO_ID_UTILS,
	BACKEND_PROTO_CTAGS,

	BACKEND_PROTO_MAX
};

struct backend_proto_item {
	unsigned long	key;
	char		*func;
	char		*file;
	char		*text;
	unsigned long	line;
};

struct backend_proto_query_params {
	union {
		struct {
			char	*name;
			int	type;
			int	nocase;
		} cscope;
		struct {
			char	*name;
		} idutils;
		struct {
			char	*name;
		} ctags;
	} u;
};

struct backend_proto {
	enum backend_protocols	protocol;
	const char		*protocol_name;
	struct list_head *	(*backend_proto_query)(struct backend_proto_query_params *params);
	int			(*backend_proto_get)(struct list_head *entry, struct backend_proto_item *item);
	int			(*backend_proto_lookup)(struct list_head *head, struct backend_proto_item *item, unsigned long key);
	void			(*backend_proto_destory)(struct list_head *head);
	void			(*backend_proto_refresh)(void);
};

struct backend_proto *backend_proto_register(struct backend_proto *proto);
void backend_proto_unregister(struct backend_proto *proto);
struct backend_proto *backend_proto_find(enum backend_protocols proto);

#endif /* BACKEND_PROTO_H */

