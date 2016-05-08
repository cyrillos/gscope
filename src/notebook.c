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

#include "notebook.h"

#include "util.h"
#include "list.h"

#include "sourceview.h"

#include "backend-proto.h"

#define NB_PAGE_LOCKED_IMG	GTK_STOCK_NO
#define NB_PAGE_UNLOCKED_IMG	GTK_STOCK_YES

#define NB_CLOSE_PAGE_RC_NAME "nb_page_close-button"
static const char nb_page_close_rc[] =
	"style \"nb_page_close-style\"\n" 
	"{\n" 
	"	GtkWidget::focus-padding = 0\n" 
	"	GtkWidget::focus-line-width = 0\n" 
	"	xthickness = 0\n" 
	"	ythickness = 0\n" 
	"}\n"
	"widget \"*.nb_page_close-button\" style \"nb_page_close-style\"";

#define NB_LOCK_PAGE_RC_NAME "nb_page_lock-button"
static const char nb_page_lock_rc[] =
	"style \"nb_page_lock-style\"\n" 
	"{\n" 
	"	GtkWidget::focus-padding = 0\n" 
	"	GtkWidget::focus-line-width = 0\n" 
	"	xthickness = 0\n" 
	"	ythickness = 0\n" 
	"}\n"
	"widget \"*.nb_page_lock-button\" style \"nb_page_lock-style\"";

void notebook_init(void)
{
	gtk_rc_parse_string(nb_page_close_rc);
	gtk_rc_parse_string(nb_page_lock_rc);
}

void notebook_fini(void)
{
	/* FIXME: Something there? */
}

static void notebook_delete_title(struct notebook_title *title)
{
	gtk_widget_destroy(title->label);
	gtk_widget_destroy(title->lock_image);
	gtk_widget_destroy(title->lock_button);
	gtk_widget_destroy(title->close_image);
	gtk_widget_destroy(title->close_button);

	memset(title, 0, sizeof(*title));
}

static void notebook_delete_page(struct notebook_page *page)
{
	GtkNotebook *nb = GTK_NOTEBOOK(page->notebook->me);
	int pos;

	/* title composite */
	notebook_delete_title(&page->title);

	/* child widget (text view and friends) */
	if (page->child) {
		/*
		 * NOTE: If child widget embedded into notebook
		 * deletes itself we should not call gtk_notebook_remove_page
		 * the GTK will delete the page by self
		 */
		if (page->child_delete_hook) {
			page->child_delete_hook(page->child, page->private);
		} else {
			pos = gtk_notebook_page_num(nb, page->child);
			if (pos != NB_ENOPAGE)
				gtk_notebook_remove_page(GTK_NOTEBOOK(page->notebook->me), pos);
		}
	}

	list_del(&page->siblings);
	free(page);
}

static void cb_page_switched(GtkNotebook *nb, GtkWidget *pg,
			guint page_num, gpointer user_data)
{
	struct notebook *notebook = (struct notebook *)user_data;
	struct notebook_page *page;
	unsigned int num;

	list_for_each_entry(page, &notebook->pages, siblings) {
		num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook->me), page->child);
		if (num == page_num) {
			if (page->page_changed_hook)
				page->page_changed_hook(page);
			else
				break;
		}
	}
}

struct notebook *notebook_create(void)
{
	struct notebook *notebook = xzalloc(sizeof(*notebook));

	notebook->me = GTK_WIDGET(gtk_notebook_new());
	INIT_LIST_HEAD(&notebook->pages);

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook->me), TRUE);

	/* signals */
	g_signal_connect(G_OBJECT(notebook->me), "switch-page",
		G_CALLBACK(cb_page_switched), (gpointer)notebook);

	return notebook;
}

void notebook_delete(struct notebook *notebook)
{
	if (!notebook)
		return;

	if (!list_empty(&notebook->pages)) {
		struct notebook_page *page, *n;
		list_for_each_entry_safe(page, n, &notebook->pages, siblings)
			notebook_delete_page(page);
	}

	gtk_widget_destroy(notebook->me);
	free(notebook);
}

static void cb_lock_page(GtkButton *widget, gpointer data)
{
	struct notebook_page *page = data;
	GtkImage *image = GTK_IMAGE(page->title.lock_image);

	gtk_image_clear(image);

	if (page->status & NB_STATUS_LOCKED) {
		gtk_image_set_from_stock(image, NB_PAGE_UNLOCKED_IMG, GTK_ICON_SIZE_MENU);
		page->status &= ~NB_STATUS_LOCKED;
	} else {
		gtk_image_set_from_stock(image, GTK_STOCK_NO, GTK_ICON_SIZE_MENU);
		page->status |= NB_STATUS_LOCKED;
	}

	gtk_widget_show_all(GTK_WIDGET(image));
}

static void cb_close_page(GtkButton *widget, gpointer data)
{
	struct notebook_page *notebook_page = data;
	if (!(notebook_page->status & NB_STATUS_LOCKED))
		notebook_delete_page((struct notebook_page *)data);
}

/* create new empty page */
struct notebook_page *notebook_create_page(struct notebook *notebook, char *title, int title_nchars, int status)
{
	struct notebook_page *page;

	page				= xzalloc(sizeof(*page));
	page->status			= status;
	page->title.container		= gtk_hbox_new(FALSE, 3);
	page->title.label		= gtk_label_new(title);
	page->title.close_button	= gtk_button_new();
	page->title.lock_button		= gtk_button_new();
	page->title.close_image		= gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	page->title.lock_image		= gtk_image_new_from_stock(NB_PAGE_UNLOCKED_IMG, GTK_ICON_SIZE_MENU);

	gtk_label_set_max_width_chars(GTK_LABEL(page->title.label), title_nchars);

	gtk_widget_set_name(page->title.close_button, NB_CLOSE_PAGE_RC_NAME);
	gtk_widget_set_name(page->title.close_button, NB_LOCK_PAGE_RC_NAME);

	gtk_button_set_relief(GTK_BUTTON(page->title.close_button), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(page->title.lock_button), GTK_RELIEF_NONE);

	gtk_box_pack_start(GTK_BOX(page->title.container), page->title.lock_button, TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(page->title.container), page->title.label, TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(page->title.container), page->title.close_button, FALSE, FALSE, 0); 

	gtk_container_add(GTK_CONTAINER(page->title.lock_button), page->title.lock_image);
	gtk_container_add(GTK_CONTAINER(page->title.close_button), page->title.close_image);

	/* buttons signals */
	g_signal_connect(page->title.lock_button, "clicked", G_CALLBACK(cb_lock_page), page);
	g_signal_connect(page->title.close_button, "clicked", G_CALLBACK(cb_close_page), page);

	gtk_widget_show_all(page->title.container);

	/* link to notebook */
	page->notebook = notebook;
	list_add(&page->siblings, &notebook->pages);

	return page;
}

void notebook_page_child_attach(struct notebook_page *page, GtkWidget *widget, void *private,
				void (*page_changed_hook)(struct notebook_page *page),
				void (*child_delete_hook)(GtkWidget *child, void *private))
{
	page->child = widget;
	page->page_changed_hook = page_changed_hook;
	page->child_delete_hook = child_delete_hook;
	page->private = private;

	gtk_notebook_append_page(GTK_NOTEBOOK(page->notebook->me), widget, page->title.container);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(page->notebook->me), widget, TRUE);
}

void notebook_page_child_dettach(struct notebook_page *page)
{
	page->child = NULL;
	page->child_delete_hook = NULL;
	page->private = NULL;
}

void notebook_activate_page(struct notebook *notebook, struct notebook_page *page)
{
	int num;
	gtk_widget_show(page->child);
	num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook->me), page->child);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook->me), num);
}

struct notebook_page *notebook_page_find(struct notebook *notebook, GtkWidget *child)
{
	struct notebook_page *page;

	list_for_each_entry(page, &notebook->pages, siblings) {
		if (page->child == child)
			return page;
	}

	return NULL;
}
