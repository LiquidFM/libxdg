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

#include <xdg/config.h>

#include <time.h>
#include "xdgapp.h"

#include "../mime/xdgmimeint.h"
#include "../containers/avltree_p.h"
#include "../containers/xdglist_p.h"


/**
 * A list of files and directories which should be watched for modifications.
 */
struct XdgFileWatcher
{
	XdgList list;
	time_t mtime;
	char path[1];
};
typedef struct XdgFileWatcher XdgFileWatcher;


/**
 * A list of XdgAppGroupEntry values.
 */
struct XdgValue
{
	XdgList list;
	char value[1];
};
typedef struct XdgValue XdgValue;

/**
 * Represents a \a ".desktop" file entry.
 */
struct XdgAppGroupEntry
{
	/**
	 * A list of \a default values.
	 */
	XdgValue *values;
	/**
	 * A tree of \a localized values.
	 */
	AvlTree localized;
};
typedef struct XdgAppGroupEntry XdgAppGroupEntry;

/**
 * Represents a \a ".desktop" file group.
 */
struct XdgAppGroup
{
	AvlTree entries;
};

/**
 * Contains a \a ".desktop" file groups of a XdgApp.
 */
struct XdgAppGroups
{
	const XdgApp *owner;
	AvlTree tree;
};
typedef struct XdgAppGroups XdgAppGroups;

/**
 * Represents contents of \a ".desktop" file.
 */
struct XdgApp
{
	XdgAppGroups *groups;
};


/**
 * A list (XdgJointList) of pointers to XdgApp.
 */
struct XdgMimeSubTypeValue
{
	XdgJointList list;
	XdgApp *app;
	char name[1];
};
typedef struct XdgMimeSubTypeValue XdgMimeSubTypeValue;

/**
 * Represents a \a sub \a type of mime type pair.
 */
struct XdgMimeSubType
{
	XdgMimeSubTypeValue *apps;
};
typedef struct XdgMimeSubType XdgMimeSubType;

/**
 * Represents a \a type of mime type pair.
 */
struct XdgMimeType
{
	AvlTree sub_types;
};
typedef struct XdgMimeType XdgMimeType;


/**
 * Initialization.
 */
void _xdg_app_init();

/**
 * Finalization.
 */
void _xdg_app_shutdown();


/**
 * Map of known associations of XdgApp with mime type.
 */
XdgMimeSubType *_xdg_mime_sub_type_add(AvlTree *map, const char *mime);
void _xdg_list_app_item_append(XdgList **list, const char *name, XdgApp *app);

#endif /* __XDG_APP_P_H_ */
