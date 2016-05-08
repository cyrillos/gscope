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

#ifndef CSCOPE_H
#define CSCOPE_H

#include "list.h"

/* warning: this is an ABI */
enum {
	CSCOPE_KEY_MIN = 0,

	CSCOPE_KEY_VERBOSE,
	CSCOPE_KEY_CROSSREF,
	CSCOPE_KEY_NOCASE,
	CSCOPE_KEY_NOCOMP,
	CSCOPE_KEY_DRY,
	CSCOPE_KEY_KERNEL,
	CSCOPE_KEY_SINGLE,
	CSCOPE_KEY_LUI,
	CSCOPE_KEY_INVERT,
	CSCOPE_KEY_FORCE,

	CSCOPE_KEY_QRY_REFERENCES,
	CSCOPE_KEY_QRY_DEFINITION,
	CSCOPE_KEY_QRY_CALLED_FUNC,
	CSCOPE_KEY_QRY_CALLING_FUNC,
	CSCOPE_KEY_QRY_TEXT,
	CSCOPE_KEY_QRY_EGREP,
	CSCOPE_KEY_QRY_FILE,
	CSCOPE_KEY_QRY_INCLUDING,

	CSCOPE_KEY_MAX
};

void init_cscope_backend_proto(void);
const char *cscope_param_desc(unsigned int opcode);

#endif /* CSCOPE_H */
