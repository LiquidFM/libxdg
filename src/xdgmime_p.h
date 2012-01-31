/* xdgmime_p.h: Private file.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Copyright (C) 2011,2012  Dmitriy Vilkov <dav.daemon@gmail.com>
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

#ifndef XDGMIME_P_H_
#define XDGMIME_P_H_


void _xdg_mime_init();
void _xdg_mime_shutdown();

/* xdg_mime_get_mime_parents() is deprecated since it does
 * not work correctly with caches. Use xdg_mime_list_parents()
 * instead, but notice that that function expects you to free
 * the array it returns.
 */
const char **xdg_mime_get_mime_parents		   (const char *mime);

/* Private versions of functions that don't call xdg_mime_init () */
int _xdg_mime_mime_type_equal(const char *mime_a, const char *mime_b);
int _xdg_mime_mime_type_subclass(const char *mime, const char *base);
const char *_xdg_mime_unalias_mime_type(const char *mime);

#endif /* XDGMIME_P_H_ */
