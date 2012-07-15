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


#if __GNUC__ < 3
    #define __builtin_expect(foo, bar) (foo)
#endif


void _xdg_list_prepend(XdgList **list, XdgList *value)
{
	if ((*list) == NULL)
	{
		(*list) = value;
		value->head = value;
		value->next = NULL;
	}
	else
	{
		XdgList *item = (*list)->head;

		value->head = value;
		value->next = item;

		do
			item->head = value;
		while (item = item->next);
	}
}

void _xdg_list_apped(XdgList **list, XdgList *value)
{
	if ((*list) == NULL)
	{
		(*list) = value;
		value->head = value;
		value->next = NULL;
	}
	else
	{
		value->head = (*list)->head;
		value->next = NULL;
		(*list)->next = value;
		(*list) = value;
	}
}

void _xdg_list_remove_if(XdgList **list, XdgListItemMatch match, void *user_data, XdgListItemFree list_item_free)
{
	if ((*list) != NULL)
	{
		XdgList *prev = NULL;
		XdgList *item = (*list)->head;

		do
			if (match(item, user_data))
				if (prev)
				{
					prev->next = item->next;
					list_item_free(item);

					if (prev->next)
					{
						item = prev->next;
						continue;
					}
					else
					{
						(*list) = prev;
						break;
					}
				}
				else
				{
					prev = item->next;
					list_item_free(item);

					if (prev)
					{
						item = prev;

						do
							item->head = prev;
						while (item = item->next);

						item = prev;
						prev = NULL;
						continue;
					}
					else
					{
						(*list) = NULL;
						break;
					}
				}
			else
				item = (prev = item)->next;
		while (item);
	}
}

void _xdg_list_free(XdgList *list, XdgListItemFree list_item_free)
{
	if (list)
	{
		XdgList *next;
		XdgList *item = list->head;

		do
		{
			next = item->next;
			list_item_free(item);
			item = next;
		}
		while (item);
	}
}

void _xdg_joint_list_apped(XdgJointList **list, XdgJointList *value)
{
	if ((*list) == NULL)
	{
		(*list) = value;
		value->head = value;
		value->next = NULL;
	}
	else
	{
		value->head = (*list)->head;
		value->next = NULL;
		(*list)->next = value;
		(*list) = value;
	}
}

const XdgList *xdg_list_begin(const XdgList *list)
{
	if (list)
		return list->head;
	else
		return NULL;
}

const XdgList *xdg_list_next(const XdgList *list)
{
	return list->next;
}

const XdgJointList *xdg_joint_list_begin(const XdgJointList *list)
{
	if (list)
		return list->head;
	else
		return NULL;
}

const XdgJointList *xdg_joint_list_next(const XdgJointList *list)
{
	/**
	 * Branch prediction:
	 * 	most of the cases list will be contain more than 1 element.
	 */
	if (__builtin_expect(list->list.next != NULL, 1))
		return (XdgJointList *)list->list.next;
	else
		return list->next;
}
