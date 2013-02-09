/** @file xdglist.h
 *  @brief Public file.
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

#ifndef XDGLIST_H_
#define XDGLIST_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a linked list.
 */
typedef struct XdgList XdgList;

/**
 * Represents an item of a linked list.
 */
typedef struct XdgListItem XdgListItem;

/**
 * Represents an item of a linked list which can be join
 * with other similar lists without memory allocation.
 */
typedef struct XdgJointListItem XdgJointListItem;

/**
 * Returns next element of a given list.
 *
 * @param item a \c "const pointer" to XdgListItem.
 * @return a \c "const pointer" to the next element of XdgList if any, or \c NULL.
 *
 * @note
 * This function does \a NOT check if a given \p "list"
 * is equal to \c NULL.
 */
const XdgListItem *xdg_list_next(const XdgListItem *item);

/**
 * Swaps elements in a linked list.
 *
 * @param item1 a \c "pointer" to XdgListItem.
 * @param item2 a \c "pointer" to XdgListItem.
 */
void xdg_list_swap(XdgListItem *item1, XdgListItem *item2);

/**
 * Removes given element of a linked list.
 *
 * @param item a \c "pointer" to XdgListItem which should be removed.
 * @return a \c "pointer" to the next element of XdgList if any, or \c NULL.
 */
XdgListItem *xdg_list_remove(XdgListItem *item);

/**
 * Returns next element of a given list.
 *
 * @param item a \c "const pointer" to XdgJointListItem.
 * @return a \c "const pointer" to the next element of XdgJointList if any, or \c NULL.
 *
 * @note
 * This function does \a NOT check if a given \p "item"
 * is equal to \c NULL.
 */
const XdgJointListItem *xdg_joint_list_next(const XdgJointListItem *item);

#ifdef __cplusplus
}
#endif

#endif /* XDGLIST_H_ */
