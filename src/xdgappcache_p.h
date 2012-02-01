/* xdgappcache_p.h: Private file.
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

#ifndef XDGAPPCACHE_P_H_
#define XDGAPPCACHE_P_H_

#include "xdgappcache.h"


struct XdgAppCahce
{
	int fd;
	char *memory;
	int size;
};


XdgAppCahce *_xdg_app_cache_new_empty(const char *file_name);
XdgAppCahce *_xdg_app_cache_new(const char *file_name);
void _xdg_app_cache_free(XdgAppCahce *cache);

#endif /* XDGAPPCACHE_P_H_ */
