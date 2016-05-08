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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "list.h"
#include "util.h"

void config_dump(struct config *config)
{
	struct config_section *config_section;
	struct config_entry *entry;

	list_for_each_entry(config_section, config->sections, siblings) {
		debug("section: %s", config_section->name);
		list_for_each_entry(entry, &config_section->entries, siblings) {
			debug("  key: %s value: %s", entry->key, entry->value);
		}
	}
}

static struct list_head *config_parse(void *mem)
{
	struct list_head *head = NULL;
	struct config_section *config_section = NULL;
	struct config_entry *entry = NULL;
	char *line;

	line = strtok(mem, "\r\n");
	do {
		line = skip_spaces(line);
		if (!line || !line[0])
			continue;

		if (line[0] == '#') {		/* comments */
			continue;
		} else if (line[0] == '[') {	/* section */
			char *end;
			line[0] = 0;
			line = skip_spaces(line + 1);
			end = strrchr(line, ']');
			if (!end) {
				error("Malformed string: %s", line);
				continue;
			}
			*end = 0;
			line = skip_spaces_rev(line);

			config_section		= xzalloc(sizeof(*config_section));
			config_section->name	= xstrdup(line);
			INIT_LIST_HEAD(&config_section->siblings);
			INIT_LIST_HEAD(&config_section->entries);
			if (!head) {
				head = xzalloc(sizeof(*head));
				INIT_LIST_HEAD(head);
			}
			list_add(&config_section->siblings, head);

		} else {			/* expect config entry */
			char *eq, *key, *value;
			if (!head) {
				error("No section defined yet: %s", line);
				continue;
			}

			eq = strchr(line, '=');
			if (eq) {
				*eq = 0, eq++;
				key = trim_word(&line);
				value = trim_word(&eq);
				if (!key)
					continue;
			} else {
				key = trim_word(&line);
				if (!key)
					continue;
				value = NULL;
			}

			entry		= xzalloc(sizeof(*entry));
			entry->key	= strdup(key);
			entry->value	= value ? strdup(value) : NULL;

			config_section = list_first_entry(head, struct config_section, siblings);
			list_add(&entry->siblings, &config_section->entries);
		}
	} while ((line = strtok(NULL, "\r\n")));

	return head;
}

struct list_head *config_read(const char *path)
{
	struct list_head *head = NULL;
	struct stat st;
	void *buf;
	int fd, err;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		error("Unable to open: %s", path);
		return NULL;
	}

	err = fstat(fd, &st);
	if (err < 0) {
		error("Unable to get stats on: %s", path);
		goto err_close;
	}

	buf = malloc(st.st_size);
	if (!buf) {
		error("Unable to allocate buffer for: %s", path);
		goto err_close;
	}

	if (read(fd, buf, st.st_size) != st.st_size) {
		error("Unable to read: %s", path);
		goto err_free;
	}

	head = config_parse(buf);

err_free:
	free(buf);
err_close:
	close(fd);
	return head;
}

void config_write(const char *path, struct config *config)
{
}

void config_free(struct config *config)
{
	struct config_section *config_section, *n;
	struct config_entry *entry, *m;

	list_for_each_entry_safe(config_section, n, config->sections, siblings) {
		list_for_each_entry_safe(entry, m, &config_section->entries, siblings) {
			free((void *)entry->key);
			if (entry->value)
				free((void *)entry->value);
			free((void *)entry);
		}
		free((void *)config_section->name);
		free((void *)config_section);
	}
	free((void *)config->sections);
	config->sections = NULL;
}
