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

#include <xdg/config.h>

#ifndef HAVE_MMAP
#	error Building exdgmime without MMAP is not yet supported.
#endif

#include "xdgapp_p.h"


struct XdgAppCahceFile
{
	int error;
	int fd;
	void *memory;
	int size;
};
typedef struct XdgAppCahceFile XdgAppCahceFile;


/**
 * Initialization of cache file
 */
void _xdg_app_cache_new_empty(XdgAppCahceFile *cache, const char *file_name);
void _xdg_app_cache_new(XdgAppCahceFile *cache, const char *file_name);
void _xdg_app_cache_close(XdgAppCahceFile *cache);


/**
 * Serialization
 */
void write_version(int fd, int version);
int read_version(void **memory);

void write_app_key(int fd, const char *key);
char *read_app_key(void **memory);

void write_app(int fd, const XdgApp *value);
void *read_app(void **memory);

void write_mime_group_type(int fd, const XdgMimeType *value);
void *read_mime_group_type(void **memory, const AvlTree *app_files_map);

void write_mime_group(int fd, const XdgMimeGroup *value);
void *read_mime_group(void **memory, const AvlTree *app_files_map);

void write_file_watcher_list(int fd, const XdgFileWatcher *list);
const XdgFileWatcher *read_file_watcher_list(void **memory);

#endif /* XDGAPPCACHE_P_H_ */
