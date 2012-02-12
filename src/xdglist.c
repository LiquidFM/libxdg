/** @internal @file xdglist.c
 *  @brief Private file.
 *
 * Data structures for storing a linked lists.
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

void _xdg_list_free(XdgList *list, XdgListItemFree list_item_free)
{
	if (list)
	{
		XdgList *next;
		XdgList *item = list->head;

		while (item)
		{
			next = item->next;
			list_item_free(item);
			item = next;
		}
	}
}

const XdgList *xdg_list_begin(const XdgList *list)
{
	if (list)
		return list->head;
	else
		return 0;
}

const XdgList *xdg_list_next(const XdgList *list)
{
	return list->next;
}
