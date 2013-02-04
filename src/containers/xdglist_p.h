/** @internal @file xdglist_p.h
 *  @brief Private file.
 *
 * Data structures for storing linked lists.
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

#ifndef XDGLIST_P_H_
#define XDGLIST_P_H_

#include "xdglist.h"


typedef void (*XdgListItemFree)(void *item);
typedef int (*XdgListItemMatch)(void *item, void *user_data);


struct XdgList
{
    XdgListItem *head;
    XdgListItem *tail;
};
typedef struct XdgList XdgList;

struct XdgListItem
{
    XdgList *list;
    XdgListItem *prev;
	XdgListItem *next;
};
typedef struct XdgListItem XdgListItem;


struct XdgJointList
{
    XdgList list;
    XdgJointListItem *head;
    XdgJointListItem *tail;
};
typedef struct XdgJointList XdgJointList;

struct XdgJointListItem
{
    XdgListItem item;
    XdgJointList *list;
	XdgJointListItem *next;
};


int _is_empty_list(const XdgList *list);
int _is_empty_jointlist(const XdgJointList *list);
void _xdg_list_prepend(XdgList *list, XdgListItem *value);
void _xdg_list_apped(XdgList *list, XdgListItem *value);
XdgListItem *_xdg_list_remove(XdgListItem *item, XdgListItemFree list_item_free);
void _xdg_list_remove_if(XdgList *list, XdgListItemMatch match, void *user_data, XdgListItemFree list_item_free);
void _xdg_list_free(XdgList *list, XdgListItemFree list_item_free);
void _xdg_joint_list_apped(XdgJointList *list, XdgJointListItem *value);


#endif /* XDGLIST_P_H_ */
