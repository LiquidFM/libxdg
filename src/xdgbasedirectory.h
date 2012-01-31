/* xdgbasedirectory.h: Private file. Base Directory Specification.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Specification:
 *  http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * Copyright (C) 2011-2012  Dmitriy Vilkov <dav.daemon@gmail.com>
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

#ifndef XDGBASEDIRECTORY_H_
#define XDGBASEDIRECTORY_H_


/* Type of callback function */
typedef int (*XdgDirectoryFunc)(const char *directory, void *user_data);

/* Calls "func" for each "XDG_DATA_" directory */
void _xdg_for_each_data_dir(XdgDirectoryFunc func, void *user_data);


#endif /* XDGBASEDIRECTORY_H_ */
