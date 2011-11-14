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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mark.h"

#include "backend-proto.h"
#include "view.h"
#include "list.h"
#include "util.h"

/* #define DBG(msg, ...) debug(msg, __VA_ARGS__) */
#define DBG(msg, ...)

/* indices on ring */
static unsigned long idx_last;
static unsigned long idx_current;

static struct mark_item marks[MARKS_LIMIT];
static int do_push = 1;

void mark_init(void)
{
	idx_last = idx_current = 0;
	memset(marks, 0, sizeof(marks));
}

void mark_fini(void)
{
	unsigned long i;

	for (i = 0; i < idx_last; i++)
		free(marks[i].file_path);

	idx_last = idx_current = 0;
}

void mark_push(struct backend_proto_item *item)
{
	if (!do_push)
		return;

	idx_last++;
	if (idx_last >= MARKS_LIMIT) {	
		idx_last = MARKS_LIMIT;
		free(marks[idx_last - 1].file_path);
	}

	DBG("%s: %s:%lu", __func__, item->file, item->line);

	idx_current = idx_last - 1;

	marks[idx_last - 1].file_path	= xstrdup(item->file);
	marks[idx_last - 1].line	= item->line;
}

static void mark_switch_to(struct gscope_info *info, unsigned long idx)
{
	struct backend_proto_item item = { 0 };

	item.file = marks[idx].file_path;
	item.line = marks[idx].line;

	do_push = 0;
	query_switch_or_read_source_into_notebook(info, &item);
	do_push = 1;
}

void mark_switch_to_next(struct gscope_info *info)
{
	DBG("%s: %lu %lu", __func__, idx_last, idx_current);

	if (idx_last && idx_current < idx_last - 1) {
		idx_current++;
		mark_switch_to(info, idx_current);
	}
}

void mark_switch_to_prev(struct gscope_info *info)
{
	DBG("%s: %lu %lu", __func__, idx_last, idx_current);

	if (idx_current > 0) {
		idx_current--;
		mark_switch_to(info, idx_current);
	}
}

