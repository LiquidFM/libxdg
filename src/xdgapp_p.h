/** @internal @file xdgapp_p.h
 *  @brief Private file.
 *
 * Data structures for storing \a ".desktop" and \a ".list" files contents.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * @copyright
 * Copyright (C) 2011,2012  Dmitriy Vilkov <dav.daemon@gmail.com>
 * @n@n
 * Licensed under the Academic Free License version 2.0
 * Or under the following terms:
 * @n@n
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * @n@n
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 * @n@n
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
#include "xdglist_p.h"


/**
 * Files which should be watched for modifications
 */
struct XdgFileWatcher
{
	XdgList list;
	time_t mtime;
	char path[1];
};
typedef struct XdgFileWatcher XdgFileWatcher;


/**
 * ".desktop" files data
 */
struct XdgValue
{
	XdgList list;
	char value[1];
};
typedef struct XdgValue XdgValue;

struct XdgEncodedValue
{
	XdgList list;
	char *encoding;
	char *value;
	char data[2];
};
typedef struct XdgEncodedValue XdgEncodedValue;

struct XdgAppGroupEntryValue
{
	XdgValue *values;
	AvlTree localized;
};
typedef struct XdgAppGroupEntryValue XdgAppGroupEntryValue;

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
struct XdgMimeSubTypeValue
{
	XdgApp *app;
	char name[1];
};
typedef struct XdgMimeSubTypeValue XdgMimeSubTypeValue;

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
XdgMimeSubType *_xdg_mime_sub_type_add(AvlTree *map, const char *mime);
void _xdg_array_app_item_add(XdgArray *array, const char *name, XdgApp *app);

#endif /* __XDG_APP_P_H_ */
