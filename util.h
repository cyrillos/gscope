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

#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Some bits are stolen from perf tool and Linux kernel :)
 */

#ifdef __GNUC__
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

#define __stringify_1(x)	#x
#define __stringify(x)		__stringify_1(x)

extern void die(const char *err, ...) NORETURN __attribute__((format (printf, 1, 2)));
extern void die_perror(const char *s) NORETURN;
extern int error(const char *err, ...) __attribute__((format (printf, 1, 2)));
extern void warning(const char *err, ...) __attribute__((format (printf, 1, 2)));
extern void debug(const char *err, ...) __attribute__((format (printf, 1, 2)));
extern void set_die_routine(void (*routine)(const char *err, va_list params) NORETURN);

#define BUG() DIE_IF(1)
#define DIE_IF(cnd)							\
do {									\
	if (cnd)							\
		die(" at (" __FILE__ ":" __stringify(__LINE__) "): "	\
		__stringify(cnd) "\n");					\
} while (0)

extern size_t strlcat(char *dest, const char *src, size_t count);

/*
 * Handy macros
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static inline void *xzalloc(unsigned long size)
{
	void *p = malloc(size);
	DIE_IF(!p);
	memset(p, 0, size);
	return p;
}

static inline void *xalloc(unsigned long size)
{
	void *p = malloc(size);
	DIE_IF(!p);
	return p;
}

static inline void *zalloc(unsigned long size)
{
	void *p = malloc(size);
	if(p)
		memset(p, 0, size);
	return p;
}

static inline void *xstrdup(char *str)
{
	char *p = strdup(str);
	DIE_IF(!p);
	return p;
}

static inline void *memdup(void *src, size_t size)
{
	void *p;
	p = xalloc(size);
	memcpy(p, src, size);
	return p;
}

#define x_isspace(x)  isspace((unsigned char)(x))

/* skip leading spaces */
static inline char *skip_spaces(const char *p)
{
	if (p)
		while (*p && x_isspace(*p))
			p++;
	return (char *)p;
}

/* skip trailing spaces */
static inline char *skip_spaces_rev(char *p)
{
	if (p) {
		size_t i = strlen(p);
		if (i-- > 0) {
			while (i && x_isspace(p[i]))
				i--;
			p[i + 1] = 0;
		}
	}
	return (char *)p;
}

static inline char *skip_word(const char *p)
{
	if (p)
		while (*p && !x_isspace(*p))
			p++;
	return (char *)p;
}

/*
 * trims spaces around the word and returns
 * pointer to the start of word, the p will
 * be set the first character after the word
 */
static inline char *trim_word(char **str)
{
	char *start, *end;

	if (!str)
		return NULL;

	start = skip_spaces(*str);
	end = skip_word(start);
	if (end != start) {
		*end = 0;
		*str = &end[1];
	}
	return start;
}

#endif /* UTIL_H */
