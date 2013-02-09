/** @internal @file xdglist.c
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

#include "xdglist_p.h"
#include <stddef.h>
#include <assert.h>


#if __GNUC__ < 3
    #define __builtin_expect(foo, bar) (foo)
#endif


int _is_empty_list(const XdgList *list)
{
    return list->head == NULL;
}

int _is_empty_jointlist(const XdgJointList *list)
{
    return list->head == NULL;
}

void _xdg_list_prepend(XdgList *list, XdgListItem *value)
{
	if (list->head == NULL)
	{
		value->list = list;
        value->prev = value->next = NULL;
		list->head = list->tail = value;
	}
	else
	{
		XdgListItem *item = list->head;

		value->list = list;
        value->prev = NULL;
		value->next = item;
		item->prev = list->head = value;
	}
}

void _xdg_list_apped(XdgList *list, XdgListItem *value)
{
	if (list->head == NULL)
	{
        value->list = list;
        value->prev = value->next = NULL;
        list->head = list->tail = value;
	}
	else
	{
		value->list = list;
		value->prev = list->tail;
		value->next = NULL;
		list->tail->next = value;
		list->tail = value;
	}
}

void _xdg_list_swap(XdgListItem *item1, XdgListItem *item2)
{
    assert(item1 != item2 && item1->list == item2->list);
    XdgListItem *item1_prev = item1->prev;
    XdgListItem *item1_next = item1->next;
    XdgListItem *item2_prev = item2->prev;
    XdgListItem *item2_next = item2->next;

    if (item2->prev = item1_prev)
        if (item2->prev == item2)
            item2->prev = item1;
        else
            item2->prev->next = item2;
    else
        item2->list->head = item2;

    if (item2->next = item1_next)
        if (item2->next == item2)
            item2->next = item1;
        else
            item2->next->prev = item2;
    else
        item2->list->tail = item2;

    if (item1->prev = item2_prev)
        if (item1->prev == item1)
            item1->prev = item2;
        else
            item1->prev->next = item1;
    else
        item1->list->head = item1;

    if (item1->next = item2_next)
        if (item1->next == item1)
            item1->next = item2;
        else
            item1->next->prev = item1;
    else
        item1->list->tail = item1;
}

XdgListItem *_xdg_list_remove(XdgListItem *item, XdgListItemFree list_item_free)
{
    XdgListItem *res;

    if (item->prev)
        if (item->prev->next = res = item->next)
            res->prev = item->prev;
        else
            item->list->tail = item->prev;
    else
        if (item->list->head = res = item->next)
            res->prev = NULL;
        else
            item->list->tail = NULL;

    list_item_free(item);
    return res;
}

void _xdg_list_remove_if(XdgList *list, XdgListItemMatch match, void *user_data, XdgListItemFree list_item_free)
{
	if (list->head)
	{
		XdgListItem *item = list->head;

		do
			if (match(item, user_data))
			    item = _xdg_list_remove(item, list_item_free);
			else
				item = item->next;
		while (item);
	}
}

void _xdg_list_free(XdgList *list, XdgListItemFree list_item_free)
{
	if (list->head)
	{
		XdgListItem *next;
		XdgListItem *item = list->head;

		do
		{
			next = item->next;
			list_item_free(item);
		}
		while (item = next);
	}

	list->head = list->tail = NULL;
}

void _xdg_joint_list_apped(XdgJointList *list, XdgJointListItem *value)
{
	if (list->head == NULL)
	{
        value->list = list;
        value->next = NULL;
        list->head = list->tail = value;
	}
	else
	{
        value->list = list;
        value->next = NULL;
        list->tail->next = value;
        list->tail = value;
	}
}

const XdgListItem *xdg_list_head(const XdgListItem *item)
{
	if (item)
		return item->list->head;
	else
		return NULL;
}

const XdgListItem *xdg_list_next(const XdgListItem *list)
{
	return list->next;
}

const XdgJointListItem *xdg_joint_list_head(const XdgJointListItem *item)
{
	if (item)
		return (const XdgJointListItem *)item->list->head->item.list->head;
	else
		return NULL;
}

const XdgJointListItem *xdg_joint_list_next(const XdgJointListItem *item)
{
	/**
	 * Branch prediction:
	 * 	most of the cases list will be contain more than 1 element.
	 */
	if (__builtin_expect(item->item.next != NULL, 1))
		return (const XdgJointListItem *)item->item.next;
	else
		if (item->next)
			return (const XdgJointListItem *)item->next->item.list->head;
		else
			return NULL;
}
