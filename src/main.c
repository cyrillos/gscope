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
 * gScope main file
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gio/gio.h>

#include "main.h"
#include "view.h"

#include "id-utils.h"
#include "cscope.h"
#include "tags.h"

#include "notebook.h"
#include "sourceview.h"
#include "queries.h"
#include "mark.h"

#include "backend-proto.h"

#include "util.h"
#include "list.h"

#include "config.h"

#include "img/gScope-32.png.img"

struct gscope_info gscope_info;

char *path_skip_base(char *path)
{
	char *p = path;
	if (!strncmp(gscope_info.working_dir, path, gscope_info.working_dir_len))
		p = (char *)g_path_skip_root(path + gscope_info.working_dir_len);
	return p;
}

///* the caller might need to free path after the call */
//static char *sanitize_path(char *path)
//{
//	if (!g_path_is_absolute(path)) {
//		char *freeme;
//		gchar *curdir = g_get_current_dir();
//		freeme = g_build_path(curdir, path, NULL);
//		path = freeme;
//		g_free(curdir);
//	}
//
//	return path;
//}

static GdkPixbuf *create_pixbuf_inline(const guint8 *data, size_t size)
{
	GError *err;
	return gdk_pixbuf_new_from_inline(size, data, FALSE, &err);
}

static void on_mnuFile_Open(struct gscope_info *info)
{
	GtkWidget *dialog;
	GSList *list, *next;
	char *path;
	struct backend_proto_item item;

	dialog = gtk_file_chooser_dialog_new("Open File",
			NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), gscope_info.working_dir);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT)
		goto out;

	list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
	while (list) {
		next = list->next;
		path = path_skip_base(list->data);
		/* we only need filename here */
		memset(&item, 0, sizeof(item));
		item.file = path;
		query_switch_or_read_source_into_notebook(&gscope_info, &item);
		g_free(list->data), list->data = NULL;
		g_slist_free_1(list);
		list = next;
	}
out:
	gtk_widget_destroy(dialog);
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *vpaned;
	GtkWidget *vbox;
	GtkWidget *menu_bar;
	GtkWidget *toolbar;
	GtkWidget *status;
	GtkWidget *notebook_source;
	GtkWidget *notebook_query;
	GdkPixbuf *pix_main;
	int status_ctx;

	gtk_set_locale();
	gtk_init(&argc, &argv);

	/* the working dir to strip from sources */
	gscope_info.working_dir = g_get_current_dir();
	DIE_IF(!gscope_info.working_dir);
	gscope_info.working_dir_len = strlen(gscope_info.working_dir);

#if 0
	{
		gscope_info.config.sections = config_read("gscope.conf");
		config_dump(&gscope_info.config);
		config_free(&gscope_info.config);
		exit(1);
	}
#endif

	/* backends init */
	
	init_id_utils_backend_proto();
	init_cscope_backend_proto();

	notebook_init();
	mark_init();

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);
	gscope_info.main_window = window;

	vpaned = gtk_vpaned_new();
	vbox = gtk_vbox_new(FALSE, 5);

	menu_bar = gtk_menu_bar_new();
	status = gtk_statusbar_new();
	gscope_info.status_bar = status;

	toolbar = gtk_toolbar_new();
	{
		GtkToolItem *item;


		item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
		gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
		g_signal_connect_swapped(GTK_OBJECT(item), "clicked", G_CALLBACK(mark_switch_to_next), (gpointer)&gscope_info);

		item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
		gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
		g_signal_connect_swapped(GTK_OBJECT(item), "clicked", G_CALLBACK(mark_switch_to_prev), (gpointer)&gscope_info);
	}

	gscope_info.notebook_source = notebook_create();
	notebook_source = gscope_info.notebook_source->me;

	gscope_info.notebook_query = notebook_create();
	notebook_query = gscope_info.notebook_query->me;

	gtk_paned_pack1(GTK_PANED(vpaned), notebook_source, TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(vpaned), notebook_query, TRUE, TRUE);

	status_ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(status), "status");
	gtk_statusbar_push(GTK_STATUSBAR(status), status_ctx, "Status bar");
	gscope_info.status_ctx = status_ctx;

	/* Menu creation */
	{
		GtkWidget *menu_file = gtk_menu_new();
		GtkWidget *mnuFile = gtk_menu_item_new_with_label("File");
		GtkWidget *mnuFile_Open = gtk_menu_item_new_with_label("Open");
		GtkWidget *mnuFile_Sep1 = gtk_separator_menu_item_new();
		GtkWidget *mnuFile_Exit = gtk_menu_item_new_with_label("Exit");

		GtkWidget *menu_project = gtk_menu_new();
		GtkWidget *mnuProject = gtk_menu_item_new_with_label("Project");
		GtkWidget *mnuProject_New = gtk_menu_item_new_with_label("New");
		GtkWidget *mnuProject_Open = gtk_menu_item_new_with_label("Open");
		GtkWidget *mnuProject_AddRemoveFiles = gtk_menu_item_new_with_label("Add/Remove Files");
		GtkWidget *mnuProject_Sep1 = gtk_separator_menu_item_new();
		GtkWidget *mnuProject_Props = gtk_menu_item_new_with_label("Properties");
		GtkWidget *mnuProject_Sep2 = gtk_separator_menu_item_new();
		GtkWidget *mnuProject_Close = gtk_menu_item_new_with_label("Close");

		GtkWidget *menu_cscope = gtk_menu_new();
		GtkWidget *mnuCScope = gtk_menu_item_new_with_label("CScope");
		GtkWidget *mnuCScope_Def = gtk_menu_item_new_with_label("Definition");
		GtkWidget *mnuCScope_Ref = gtk_menu_item_new_with_label("References");
		GtkWidget *mnuCScope_Called = gtk_menu_item_new_with_label("Called By");
		GtkWidget *mnuCScope_Calling = gtk_menu_item_new_with_label("Calling");
		GtkWidget *mnuCScope_Sep = gtk_separator_menu_item_new();
		GtkWidget *mnuCScope_Rebuild = gtk_menu_item_new_with_label("Rebuild");

		/*
		 * File
		 */
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), mnuFile_Open);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), mnuFile_Sep1);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_file), mnuFile_Exit);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mnuFile), menu_file);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mnuFile);

		menu_bind_handler(mnuFile_Open, on_mnuFile_Open, &gscope_info);
		menu_bind_handler(mnuFile_Exit, gtk_main_quit, NULL);

		/*
		 * Project
		 */
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_project), mnuProject_New);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_project), mnuProject_Open);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_project), mnuProject_AddRemoveFiles);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_project), mnuProject_Sep1);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_project), mnuProject_Props);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_project), mnuProject_Sep2);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_project), mnuProject_Close);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mnuProject), menu_project);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mnuProject);

		/*
		 * CScope
		 */
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_cscope), mnuCScope_Def);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_cscope), mnuCScope_Ref);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_cscope), mnuCScope_Called);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_cscope), mnuCScope_Calling);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_cscope), mnuCScope_Sep);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_cscope), mnuCScope_Rebuild);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mnuCScope), menu_cscope);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mnuCScope);

		menu_bind_handler(mnuCScope_Def, do_cscope_query_dialog, (gpointer)CSCOPE_KEY_QRY_DEFINITION);
		menu_bind_handler(mnuCScope_Ref, do_cscope_query_dialog, (gpointer)CSCOPE_KEY_QRY_REFERENCES);
		menu_bind_handler(mnuCScope_Called, do_cscope_query_dialog, (gpointer)CSCOPE_KEY_QRY_CALLED_FUNC);
		menu_bind_handler(mnuCScope_Calling, do_cscope_query_dialog, (gpointer)CSCOPE_KEY_QRY_CALLING_FUNC);
		menu_bind_handler(mnuCScope_Rebuild, do_cscope_rebuild, NULL);
	}

	gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), vpaned, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), status, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	pix_main = create_pixbuf_inline(gScope_32_img_inline, sizeof(gScope_32_img_inline));

	gtk_window_set_title(GTK_WINDOW(window), "gScope");
	gtk_window_set_icon(GTK_WINDOW(window), pix_main);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	gtk_statusbar_push(GTK_STATUSBAR(status), 0, gscope_info.working_dir);

	gtk_widget_show_all(window);

	g_signal_connect(window, "destroy",
			G_CALLBACK(gtk_main_quit), &window);

	gtk_main();

	return 0;
}
