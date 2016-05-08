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
#include <gio/gio.h>

#include "view.h"

#include "backend-proto.h"
#include "id-utils.h"
#include "cscope.h"
#include "tags.h"

#include "notebook.h"
#include "sourceview.h"
#include "queries.h"

#include "util.h"
#include "list.h"

static void cb_notebook_page_switched(struct notebook_page *page)
{
	struct source_view *source_view;

	if (!(page->status & NB_STATUS_SOURCE))
		return;

	source_view = page->private;

	gscope_info.title_buf[0] = 0;
	strlcat(gscope_info.title_buf, source_view->file_path, sizeof(gscope_info.title_buf));

	gtk_window_set_title(GTK_WINDOW(gscope_info.main_window), gscope_info.title_buf);
}

void read_file_into_notebook(struct gscope_info *info, char *filename)
{
	struct notebook_page *page;
	struct source_view *source_view;

	source_view = source_view_create(filename);
	DIE_IF(!source_view);

	page = notebook_create_page(info->notebook_source, g_path_get_basename(filename),
			strlen(g_path_get_basename(filename)), NB_STATUS_SOURCE);
	notebook_page_child_attach(page, source_view->me, source_view, cb_notebook_page_switched, source_view->delete_hook);
	notebook_activate_page(info->notebook_source, page);
	gtk_widget_show_all(info->main_window);
}

void do_cscope_query(struct gscope_info *info, char *query_name, int type, int nocase)
{
	struct notebook_page *page;
	struct list_head *head;
	struct query_result query_result;
	struct query_view *query_view;

	struct backend_proto_query_params params = {
		.u.cscope.name		= query_name,
		.u.cscope.type		= type,
		.u.cscope.nocase	= nocase,
	};

	query_result.proto = backend_proto_find(BACKEND_PROTO_CSCOPE);
	DIE_IF(!query_result.proto);

	head = query_result.proto->backend_proto_query(&params);
	if (!head || list_empty(head))
		return;
	query_result.head = head;

	/*
	 * A special case -- single entry
	 */
	if (list_is_singular(head)) {
		struct backend_proto_item item;
		struct list_head *t;
		list_for_each(t, head)
			query_result.proto->backend_proto_get(t, &item);
		query_switch_or_read_source_into_notebook(info, &item);
	} else {
		query_view = query_view_create(&query_result);
		DIE_IF(!query_view);

		page = notebook_create_page(info->notebook_query, query_name, strlen(query_name), NB_STATUS_QUERY);
		notebook_page_child_attach(page, query_view->me, query_view, NULL, query_view_destroy);

		gtk_widget_show_all(info->main_window);
	}
}

void do_id_utils_query(struct gscope_info *info, char *query_name)
{
	struct notebook_page *page;
	struct list_head *head;
	struct query_result query_result;
	struct query_view *query_view;

	struct backend_proto_query_params params = {
		.u.idutils.name		= query_name,
	};

	query_result.proto = backend_proto_find(BACKEND_PROTO_ID_UTILS);
	DIE_IF(!query_result.proto);

	head = query_result.proto->backend_proto_query(&params);
	if (!head || list_empty(head))
		return;
	query_result.head = head;

	query_view = query_view_create(&query_result);
	DIE_IF(!query_view);

	page = notebook_create_page(info->notebook_query, query_name, 8, NB_STATUS_QUERY);
	notebook_page_child_attach(page, query_view->me, query_view, NULL, query_view_destroy);

	gtk_widget_show_all(info->main_window);
}

struct cscope_cb_params {
	GtkWidget *entry;
	unsigned int opcode;
};

static void cb_response(GtkWidget *w, gint resp_code, gpointer param)
{
	if (resp_code == GTK_RESPONSE_OK) {
		GtkWidget *entry = ((struct cscope_cb_params *)param)->entry;
		unsigned int opcode = ((struct cscope_cb_params *)param)->opcode;
		do_cscope_query(&gscope_info, (char *)gtk_entry_get_text(GTK_ENTRY(entry)),
				opcode, 0);
	}
	gtk_widget_destroy(w);
}

static struct cscope_cb_params params;

void do_cscope_query_dialog(gpointer opcode)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entry;
	GtkEntryBuffer *buffer;
	const char *desc = cscope_param_desc((unsigned int)(unsigned long)opcode);

	if (!desc)
		return;

	window = gtk_dialog_new_with_buttons(desc, GTK_WINDOW(gscope_info.main_window), 0,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
					NULL);

	gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
	gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "Enter the symbol");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* Create a buffer */
	buffer = gtk_entry_buffer_new(NULL, 0);

	/* Create our first entry */
	entry = gtk_entry_new_with_buffer(buffer);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

	params.opcode	= (unsigned int)(unsigned long)opcode;
	params.entry	= entry;

	g_signal_connect(window, "response", G_CALLBACK(cb_response), &params);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroyed), &window);

	g_object_unref(buffer);

	gtk_widget_show_all(window);
}

void do_cscope_rebuild(gpointer pointer)
{
	struct backend_proto *proto = backend_proto_find(BACKEND_PROTO_CSCOPE);
	DIE_IF(!proto);

	proto->backend_proto_refresh();
}
