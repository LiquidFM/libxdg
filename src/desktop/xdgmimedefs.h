/* xdgmimedefs.h: Private file.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Copyright (C) 2011  Dmitriy Vilkov <dav.daemon@gmail.com>
 *
 * Licensed under the Academic Free License version 2.0
 * Or under the following terms:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __XDG_MIME_DEFS_H_
#define __XDG_MIME_DEFS_H_

#include "../mime/xdgmimeint.h"

#define BOOL int

#define READ_FROM_FILE_BUFFER_SIZE 1024
#define MIME_TYPE_NAME_BUFFER_SIZE 128
#define LOCALE_NAME_BUFFER_SIZE    64

#define REMOVE_WHITE_SPACES_LEFT(line, ptr) \
	do { *ptr = 0; --ptr; } while(ptr > line && (*ptr) == ' ');

#define REMOVE_WHITE_SPACES_RIGHT(ptr) \
	while (*ptr == ' ' && *ptr != '\n') ++ptr;

#endif /* __XDG_MIME_DEFS_H_ */
