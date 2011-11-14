/*
 * (c) 2011 Cyrill Gorcunov, gorcunov@gmail.com
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

#ifndef MARK_H
#define MARK_H

#include "list.h"

struct gscope_info;
struct backend_proto_item;

#define MARKS_LIMIT (1 << 12)

struct mark_item {
	char		*file_path;
	unsigned long	line;
};

void mark_init(void);
void mark_fini(void);

void mark_push(struct backend_proto_item *item);
void mark_switch_to_prev(struct gscope_info *info);
void mark_switch_to_next(struct gscope_info *info);

#endif /* MARK_H */
