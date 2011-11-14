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

#ifndef QUERIES_H
#define QUERIES_H

#include <gtk/gtk.h>

#include "list.h"

#include "backend-proto.h"
#include "notebook.h"
#include "tags.h"
#include "id-utils.h"
#include "cscope.h"
#include "view.h"

struct query_result {
	struct list_head	*head;
	enum backend_protocols	protocol;
	struct backend_proto	*proto;
};

struct query_view {
	GtkWidget		*me;		/* master widget we are embedded into */

	struct query_result	query;
	GtkWidget		*query_tree_widget;
	GtkListStore		*query_store;
};

static inline int is_valid_query_result(struct query_result *result)
{
	return result->proto != NULL;
}

struct query_view *query_view_create(struct query_result *result);
void query_view_destroy(GtkWidget *me, void *private);

struct gscope_info;
int query_switch_or_read_source_into_notebook(struct gscope_info *info, struct backend_proto_item *item);

#endif /* QUERIES_H */
