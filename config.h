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
 * config manager
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "list.h"

struct config_entry {
	struct list_head siblings;
	const char *key;
	const char *value;
};

struct config_section {
	const char *name;
	struct list_head siblings;
	struct list_head entries;
};

struct config {
	struct list_head *sections;
};

struct list_head *config_read(const char *path);
void config_write(const char *path, struct config *config);
void config_free(struct config *config);
void config_dump(struct config *config);

#endif /* CONFIG_H */
