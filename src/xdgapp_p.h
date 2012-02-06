/* xdgmimeapp_p.h: Private file.  Datastructure for storing
 * the .desktop and .list files.
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

#ifndef __XDG_APP_P_H_
#define __XDG_APP_P_H_

#include <time.h>
#include "xdgapp.h"
#include "avltree.h"
#include "xdgarray_p.h"


/**
 * Files which should be watched for modifications
 */
typedef struct XdgFileWatcher XdgFileWatcher;
struct XdgFileWatcher
{
	XdgFileWatcher *head;
	XdgFileWatcher *next;
	time_t mtime;
	char path[1];
};


/**
 * ".desktop" files data
 */
struct XdgAppGroupEntry
{
	XdgArray values;
};
typedef struct XdgAppGroupEntry XdgAppGroupEntry;

struct XdgAppGroup
{
	AvlTree entries;
};

struct XdgApp
{
	AvlTree groups;
};


/**
 * ".list" files data
 */
struct XdgMimeSubType
{
	XdgArray apps;
};
typedef struct XdgMimeSubType XdgMimeSubType;

struct XdgMimeType
{
	AvlTree sub_types;
};
typedef struct XdgMimeType XdgMimeType;

struct XdgMimeGroup
{
	AvlTree types;
};
typedef struct XdgMimeGroup XdgMimeGroup;


/**
 * Initialization
 */
void _xdg_app_init();
void _xdg_app_shutdown();


/**
 * Map of known associations of XdgApp with mime type
 */
XdgMimeSubType *xdg_mime_sub_type_add(AvlTree *map, const char *mime);

#endif /* __XDG_APP_P_H_ */
