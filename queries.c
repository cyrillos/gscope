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
 * handle queries (id-utils, cscope, text, etc...)
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

#include <gtk/gtk.h>

#include "queries.h"
#include "notebook.h"
#include "tags.h"
#include "backend-proto.h"
#include "sourceview.h"
#include "view.h"

#include "util.h"
#include "list.h"

#include "mark.h"

enum {
	COL_FILE,
	COL_LINE,
	COL_FUNC,
	COL_INDEX,
	NUM_COLS
};

int query_switch_or_read_source_into_notebook(struct gscope_info *info, struct backend_proto_item *item)
{
	struct notebook_page *page;
	struct source_view *sv;
	int attempt = 2;

	while (attempt--) {
		/*
		 * search thru all sourceview pages
		 * and highlight the name found
		 */
		//debug("---");
		list_for_each_entry(page, &info->notebook_source->pages, siblings) {
			sv = (struct source_view *)page->private;
			//debug("(%s : %s) : %s", sv->file_name, sv->file_path, item->file);
			if (!strcmp(item->file, sv->file_path)) {
				mark_push(item);
				source_view_scroll_to_line(sv, item->line);
				notebook_activate_page(info->notebook_source, page);
				return 0;
			}
		}
		/* not found, create new one */
		read_file_into_notebook(info, item->file);
	}
	warning("Too many attempts too read file: %s", item->file);
	return -1;
}

static int item_from_row(GtkTreeView *view, GtkTreePath *path, struct query_result *query,
			struct backend_proto_item *item)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(view);
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		unsigned long key;
		gtk_tree_model_get(model, &iter, COL_INDEX, &key, -1);
		if (query->proto->backend_proto_lookup(query->head, item, key)) {
			warning("%s: Lookup for key %lu failed",
				query->proto->protocol_name, key);
			return -1;
		}
		/* FIXME: free or unreference */
	}

	return 0;
}

static gboolean
on_query_tree_widget_button_press_event(GtkWidget *widget,
					GdkEventButton *event,
					gpointer data)
{
	struct query_view *query_view = (struct query_view *)data;
	GtkTreeViewColumn *focus_column = NULL;
	GtkTreePath *path = NULL;

	if (event->button != 1 || event->type != GDK_2BUTTON_PRESS)
		return FALSE;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(query_view->query_tree_widget), &path, &focus_column);
	if (path) {
		struct notebook_page *page;
		struct source_view *sv;
		struct backend_proto_item item;
		if (item_from_row(GTK_TREE_VIEW(widget), path, &query_view->query, &item)) {
			//debug("item_from_row failed");
			goto out;
		}
		query_switch_or_read_source_into_notebook(&gscope_info, &item);
out:
		gtk_tree_path_free(path);
	}

	return FALSE;
}

struct query_view *query_view_create(struct query_result *query_result)
{
	GtkWidget *scroll_tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GtkTreeIter iter;

	struct query_view *query_view;
	struct backend_proto_item item;
	struct list_head *t;

	if (!(is_valid_query_result(query_result)))
		return NULL;

	query_view = xzalloc(sizeof(*query_view));
	memcpy(&query_view->query, query_result, sizeof(query_view->query));

	/*
	 * GUI things
	 */
	query_view->me = gtk_hpaned_new();

	scroll_tree = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_tree),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	query_view->query_tree_widget = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroll_tree), query_view->query_tree_widget);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("File",
				renderer, "text", COL_FILE, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_FILE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(query_view->query_tree_widget), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Line",
				renderer, "text", COL_LINE, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_LINE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(query_view->query_tree_widget), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Text",
				renderer, "text", COL_FUNC, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_FUNC);
	gtk_tree_view_append_column(GTK_TREE_VIEW(query_view->query_tree_widget), column);

	query_view->query_store = gtk_list_store_new(NUM_COLS,
					G_TYPE_STRING, G_TYPE_UINT,
					G_TYPE_STRING, G_TYPE_ULONG);

	list_for_each(t, query_result->head) {
		int err = query_result->proto->backend_proto_get(t, &item);
		if (err) {
			error("%s: backend_proto_get returned %d",
				query_result->proto->protocol_name, err);
			break;
		}
		gtk_list_store_append(query_view->query_store, &iter);
		gtk_list_store_set(query_view->query_store, &iter,
				COL_FILE, item.file,
				COL_LINE, item.line,
				COL_FUNC, item.text,
				COL_INDEX,item.key,
				-1);
	}

	model = GTK_TREE_MODEL(query_view->query_store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(query_view->query_tree_widget), model);

	/* there is own reference anyway */
	g_object_unref(model);

	gtk_paned_pack1(GTK_PANED(query_view->me), scroll_tree, TRUE, FALSE);

	/* bind events */
	g_signal_connect(G_OBJECT(query_view->query_tree_widget), "button_press_event",
			G_CALLBACK(on_query_tree_widget_button_press_event),
			(gpointer)query_view);

	return query_view;
}

/* we don't use 'me' at moment */
void query_view_destroy(GtkWidget *me, void *private)
{
	struct query_view *query_view = private;

	//gtk_widget_destroy(query_view->query_store);
	gtk_widget_destroy(query_view->query_tree_widget);
	gtk_widget_destroy(query_view->me);

	query_view->query.proto->backend_proto_destory(query_view->query.head);
	free(query_view);
}
