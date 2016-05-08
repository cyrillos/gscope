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

#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include <gtk/gtk.h>

#include "list.h"

#define NB_STATUS_LOCKED	(1 << 0)
#define NB_STATUS_SOURCE	(1 << 1)
#define NB_STATUS_QUERY		(1 << 2)

#define NB_ENOPAGE		-2

struct notebook {
	GtkWidget	*me;		/* this notebook */
	struct list_head pages;		/* all pages in notebook */
	void		*private;	/* auxilary */
};

struct notebook_title {
	GtkWidget	*container;	/* other widgets embedded in */
	GtkWidget	*label;		/* notebook title string */
	GtkWidget	*lock_button;	/* page locking specifics */
	GtkWidget	*lock_image;
	GtkWidget	*close_button;	/* page closing specifics */
	GtkWidget	*close_image;
};

struct notebook_page {
	struct list_head siblings;	/* all pages in notebook */

	struct notebook *notebook;	/* whom we belong to */
	struct notebook_title title;	/* title specifics */
	GtkWidget	*parent;	/* which object we're embedded */
	GtkWidget	*child;		/* content of this page */
	void		*private;	/* private for hooks */
	void		(*child_delete_hook)(GtkWidget *child, void *private);
	void		(*page_changed_hook)(struct notebook_page *page);
	int		status;
};

void notebook_init(void);
void notebook_fini(void);

struct notebook *notebook_create(void);
void notebook_delete(struct notebook *notebook);
struct notebook_page *notebook_create_page(struct notebook *notebook, char *title, int title_nchars, int status);
void notebook_page_child_attach(struct notebook_page *page, GtkWidget *widget, void *private,
				void (*page_changed_hook)(struct notebook_page *page),
				void (*child_delete_hook)(GtkWidget *child, void *private));
void notebook_page_child_dettach(struct notebook_page *page);
void notebook_activate_page(struct notebook *notebook, struct notebook_page *page);

struct notebook_page *notebook_page_find(struct notebook *notebook, GtkWidget *child);

#endif /* NOTEBOOK_H */
