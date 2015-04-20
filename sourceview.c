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
 * source view handling
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
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>

#include "sourceview.h"
#include "tags.h"

#include "util.h"
#include "list.h"

#include "queries.h"
#include "view.h"

enum {
	COL_NAME,
	COL_LINE,
	COL_TYPE,
	COL_INDEX,
	NUM_COLS
};

void source_view_scroll_to_line(struct source_view *sv, unsigned long line)
{
	if (!line)
		line = 1;

	/*
	 * for a caller @line is starting from 1 but in turn
	 * internally they are counted from 0 in widget
	 */
	gtk_text_buffer_get_iter_at_line(sv->text_buffer, &sv->text_iter_start, line - 1);
	gtk_text_buffer_get_iter_at_line(sv->text_buffer, &sv->text_iter_end, line);

	gtk_text_buffer_select_range(sv->text_buffer, &sv->text_iter_start, &sv->text_iter_end);

	while(gtk_events_pending())
		gtk_main_iteration();

	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(sv->text_widget), &sv->text_iter_start, 0.25, FALSE, 0, 0);
}

static int source_view_current_line(struct source_view *sv)
{
	gtk_text_buffer_get_selection_bounds(sv->text_buffer,
			&sv->text_iter_start, &sv->text_iter_end);
	return gtk_text_iter_get_line(&sv->text_iter_start);
}

static struct tag *tag_from_row(GtkTreeView *view, GtkTreePath *path, struct list_head *tags)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	struct tag *t = NULL;

	model = gtk_tree_view_get_model(view);
	if (gtk_tree_model_get_iter(model, &iter, path)) {
		unsigned long key;
		gtk_tree_model_get(model, &iter, COL_INDEX, &key, -1);
		t = tag_key_lookup(tags, key);
		/* FIXME: free or unreference */
	}

	return t;
}

static gboolean on_tagsTree_button_press_event(GtkWidget *widget,
			GdkEventButton *event, gpointer data)
{
	struct source_view *sv = (struct source_view *)data;
	GtkTreeViewColumn *focus_column = NULL;
	GtkTreePath *path = NULL;

	if (event->button != 1 || event->type != GDK_2BUTTON_PRESS)
		return FALSE;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(widget), &path, &focus_column);
	if (path) {
		struct list_head *list = sv->etags;
		struct tag *t = tag_from_row(GTK_TREE_VIEW(widget), path, list);
		if (t)
			source_view_scroll_to_line(sv, t->line);
		gtk_tree_path_free(path);
	}

	return FALSE;
}

static void source_view_delete_hook(GtkWidget *child, void *private)
{
	struct source_view *sv = private;
	if (!sv)
		return;

	gtk_list_store_clear(sv->etags_store);

	gtk_widget_destroy(sv->text_widget);
	gtk_widget_destroy(sv->etags_tree_widget);
	gtk_widget_destroy(sv->me);

	g_free(sv->text_contents);
	tags_free(sv->etags);
	free(sv->file_path);
	free(sv);
}

static void __cb_popup_cscope_query(struct source_view *sv, int type, int nocase)
{
	gchar *str;
	if (gtk_text_buffer_get_selection_bounds(sv->text_buffer,
		&sv->text_iter_start, &sv->text_iter_end)) {
		str = gtk_text_buffer_get_slice(sv->text_buffer,
				&sv->text_iter_start, &sv->text_iter_end,
				FALSE);
		str = skip_spaces(str);
		if (!str || !str[0])
			return;

		do_cscope_query(&gscope_info, str, type, nocase);
	}
}

static void cp_popup_cscope_calling(gpointer user_data, void *dummy)
{
	struct source_view *sv = (struct source_view *)user_data;
	__cb_popup_cscope_query(sv, CSCOPE_KEY_QRY_CALLING_FUNC, 0);
}

static void cp_popup_cscope_called(gpointer user_data, void *dummy)
{
	struct source_view *sv = (struct source_view *)user_data;
	__cb_popup_cscope_query(sv, CSCOPE_KEY_QRY_CALLED_FUNC, 0);
}

static void cp_popup_cscope_ref(gpointer user_data, void *dummy)
{
	struct source_view *sv = (struct source_view *)user_data;
	__cb_popup_cscope_query(sv, CSCOPE_KEY_QRY_REFERENCES, 0);
}

static void cp_popup_cscope_def(gpointer user_data, void *dummy)
{
	struct source_view *sv = (struct source_view *)user_data;
	__cb_popup_cscope_query(sv, CSCOPE_KEY_QRY_DEFINITION, 0);
}

static void cb_populate_popup(GtkTextView *entry, GtkMenu *menu, gpointer user_data)
{
	GtkWidget *mnuSep = gtk_separator_menu_item_new();
	GtkWidget *mnuCScope_Def = gtk_menu_item_new_with_label("Definition");
	GtkWidget *mnuCScope_Ref = gtk_menu_item_new_with_label("References");
	GtkWidget *mnuCScope_Called = gtk_menu_item_new_with_label("Called By");
	GtkWidget *mnuCScope_Calling = gtk_menu_item_new_with_label("Calling");

#define popup_push(mnu, callback)						\
	do {									\
		gtk_menu_shell_insert(GTK_MENU_SHELL(menu), mnu, 0);		\
		gtk_widget_show(mnu);						\
		g_signal_connect_swapped(GTK_OBJECT(mnu), "activate",		\
			G_CALLBACK(callback), (gpointer)user_data);		\
	} while (0)

	gtk_menu_shell_insert(GTK_MENU_SHELL(menu), mnuSep, 0);
	gtk_widget_show(mnuSep);

	popup_push(mnuCScope_Calling,	cp_popup_cscope_calling);
	popup_push(mnuCScope_Called,	cp_popup_cscope_called);
	popup_push(mnuCScope_Ref,	cp_popup_cscope_ref);
	popup_push(mnuCScope_Def,	cp_popup_cscope_def);
}

static gboolean cb_press_key(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	struct source_view *sv = (struct source_view *)user_data;

	//debug("event->state: %x event->keyval: %x", event->state, event->keyval);

	// GDK_Control_L, GDK_bracketleft

	switch (event->keyval) {
	case GDK_bracketleft:
		if (!(event->state & GDK_CONTROL_MASK))
			break;
		break;
	case GDK_bracketright:
		if (!(event->state & GDK_CONTROL_MASK))
			break;
		break;
	case GDK_W:
	case GDK_w:
		if (event->state & GDK_CONTROL_MASK) {
			struct notebook_page *page;
			page = notebook_page_find(gscope_info.notebook_source, sv->me);
			if (page)
				g_signal_emit_by_name(page->title.close_button, "clicked");
			/* note after this point there is no user-data anymore */
		}
		break;
	case GDK_E:
	case GDK_e:
		{
			char *argv[5];
			char line[16];
			int pout;
			GError *error = NULL;

			if (!(event->state &
			      (GDK_CONTROL_MASK | GDK_SHIFT_MASK)))
				break;

			if (event->state & GDK_SHIFT_MASK) {
				argv[0] = (char *)"gvim";
				argv[1] = sv->file_path;
				argv[2] = line;
				argv[3] = NULL;
			} else {
				argv[0] = (char *)"gvim";
				argv[1] = (char *)"-R";
				argv[2] = sv->file_path;
				argv[3] = line;
				argv[4] = NULL;
			}

			snprintf(line, sizeof(line), "+%d",
				source_view_current_line(sv) + 1);

			g_spawn_async_with_pipes(NULL, argv, NULL,
					G_SPAWN_SEARCH_PATH,
					NULL, NULL, NULL,
					NULL, &pout, NULL, &error);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/*
 * creates tags tree and textbox (both filled
 * owb way by a content from file @path) and
 * embed them into container
 */
struct source_view *source_view_create(char *path)
{
	GtkWidget *scroll_tree;
	GtkWidget *scroll_text;
	PangoFontDescription *font_desc;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GError *err;

	char *font_name;
	struct tag *t;
	size_t size;

	struct source_view *sv = xzalloc(sizeof(*sv));

	/*
	 * parse all things first
	 */
	sv->etags = tag_parse_file(path);
	if (!sv->etags)
		goto out_free;
	sv->file_path = strdup(path);
	sv->file_name = g_path_get_basename(sv->file_path);
	if (!sv->file_path)
		goto out_free_dups;

	if (!g_file_get_contents(path, &sv->text_contents, &size, &err))
		goto out_free_etags;

	/*
	 * Now GUI things
	 */
	sv->me = gtk_hpaned_new();

	scroll_tree = gtk_scrolled_window_new(NULL, NULL);
	scroll_text = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_tree),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_text),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/*
	 * etags Tree
	 */
	sv->etags_tree_widget = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroll_tree), sv->etags_tree_widget);
	gtk_widget_set_size_request(scroll_tree, 20, -1);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Name",
				renderer, "text", COL_NAME, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(sv->etags_tree_widget), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Line",
				renderer, "text", COL_LINE, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_LINE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(sv->etags_tree_widget), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Type",
				renderer, "text", COL_TYPE, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, COL_TYPE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(sv->etags_tree_widget), column);

	sv->etags_store = gtk_list_store_new(NUM_COLS,
				G_TYPE_STRING, G_TYPE_UINT,
				G_TYPE_STRING, G_TYPE_ULONG);

	list_for_each_entry(t, sv->etags, list) {
		gtk_list_store_append(sv->etags_store, &iter);
		gtk_list_store_set(sv->etags_store, &iter,
			COL_NAME, t->name,
			COL_LINE, t->line,
			COL_TYPE, t->symtype,
			COL_INDEX,t->key,
			-1);
	}

	model = GTK_TREE_MODEL(sv->etags_store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(sv->etags_tree_widget), model);

	/* there is own reference anyway */
	g_object_unref(model);

	/* bind events */
	g_signal_connect(G_OBJECT(sv->etags_tree_widget), "button_press_event",
			G_CALLBACK(on_tagsTree_button_press_event),
			(gpointer)sv);

	/*
	 * Text widget
	 */
	sv->text_widget = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(scroll_text), sv->text_widget);

	font_name = getenv("GSCOPE_SOURCE_FONT");
	font_desc = pango_font_description_from_string(font_name ? : "Droid Sans Mono 11");
	gtk_widget_modify_font(sv->text_widget, font_desc);
	pango_font_description_free(font_desc);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(sv->text_widget), FALSE);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(sv->text_widget), 5);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(sv->text_widget), 5);

	sv->text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sv->text_widget));
	gtk_text_buffer_set_text(sv->text_buffer, sv->text_contents, -1);

	gtk_text_buffer_get_iter_at_line(sv->text_buffer, &sv->text_iter_start, 1);
	gtk_text_buffer_get_iter_at_line(sv->text_buffer, &sv->text_iter_end, 1);

	gtk_paned_pack1(GTK_PANED(sv->me), scroll_tree, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(sv->me), scroll_text, TRUE, FALSE);

	/*
	 * The popup we need
	 */
	g_signal_connect(G_OBJECT(sv->text_widget),
				"populate-popup",
				G_CALLBACK(&cb_populate_popup),
				(gpointer)sv);

	/*
	 * Editing -- Ctrl+E
	 */
	g_signal_connect(G_OBJECT(sv->text_widget),
				"key-press-event",
				G_CALLBACK(&cb_press_key),
				(gpointer)sv);

	sv->delete_hook = &source_view_delete_hook;

	return sv;

out_free_etags:
	tags_free(sv->etags);
out_free_dups:
	free(sv->file_path);
out_free:
	free(sv);
	return NULL;
}
