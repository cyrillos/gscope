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

#ifndef VIEW_H
#define VIEW_H

#include <gtk/gtk.h>

#include "id-utils.h"
#include "cscope.h"
#include "queries.h"
#include "config.h"

/*
 * Some handy macros
 */
#define menu_bind_handler(menu, proc, data)				\
	g_signal_connect_swapped(GTK_OBJECT(menu), "activate",		\
				G_CALLBACK(proc), (gpointer)data)

struct gscope_info {
	GtkWidget		*main_window;
	GtkWidget		*status_bar;
	int			status_ctx;

	char			title_buf[512];
	char			*working_dir;
	unsigned int		working_dir_len;

	struct config		config;

	struct notebook		*notebook_source;	/* source code notebook */
	struct notebook		*notebook_query;	/* queries notebook */
};

extern struct gscope_info gscope_info;

void read_file_into_notebook(struct gscope_info *info, char *filename);

void do_id_utils_query(struct gscope_info *info, char *query_name);
void do_cscope_query(struct gscope_info *info, char *query_name, int type, int nocase);

void do_cscope_query_dialog(gpointer opcode);
void do_cscope_rebuild(gpointer pointer);

#endif /* VIEW_H */
