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

#ifndef SOURCEVIEW_H
#define SOURCEVIEW_H

#include "list.h"

struct source_view {
	GtkWidget		*me;		/* master widget we are embedded into */

	GtkWidget		*text_widget;
	GtkTextBuffer		*text_buffer;
	char			*text_contents;
	GtkTextIter		text_iter_start,
				text_iter_end;

	struct list_head	*etags;
	GtkWidget		*etags_tree_widget;
	GtkListStore		*etags_store;

	char			*file_path;
	char			*file_name;
	void			*private;

	void			(*delete_hook)(GtkWidget *child, void *private);
};

struct source_view *source_view_create(char *path);
void source_view_scroll_to_line(struct source_view *sv, unsigned long line);

#endif /* SOURCEVIEW_H */
