/** @file xdglist.h
 *  @brief Public file.
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

#ifndef XDGLIST_H_
#define XDGLIST_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a linked list.
 */
typedef struct XdgList      XdgList;

/**
 * Represents a linked list which could be join
 * with other similar lists without memory allocation.
 */
typedef struct XdgJointList XdgJointList;


/**
 * Returns head of a given list.
 *
 * @param list a \c "const pointer" to XdgList.
 * @return a \c "const pointer" to head of XdgList.
 *
 * @note
 * This function checks if a given \p "list"
 * is equal to \c NULL and returns \c NULL if it is.
 */
const XdgList *xdg_list_begin(const XdgList *list);

/**
 * Returns next element of a given list.
 *
 * @param list a \c "const pointer" to XdgList.
 * @return a \c "const pointer" to next element of XdgList.
 *
 * @note
 * This function does \a NOT check if a given \p "list"
 * is equal to \c NULL.
 */
const XdgList *xdg_list_next(const XdgList *list);

/**
 * Returns head of a given list.
 *
 * @param list a \c "const pointer" to XdgJointList.
 * @return a \c "const pointer" to head of XdgJointList.
 *
 * @note
 * This function checks if a given \p "list"
 * is equal to \c NULL and returns \c NULL if it is.
 */
const XdgJointList *xdg_joint_list_begin(const XdgJointList *list);

/**
 * Returns next element of a given list.
 *
 * @param list a \c "const pointer" to XdgJointList.
 * @return a \c "const pointer" to next element of XdgJointList.
 *
 * @note
 * This function does \a NOT check if a given \p "list"
 * is equal to \c NULL.
 */
const XdgJointList *xdg_joint_list_next(const XdgJointList *list);

#ifdef __cplusplus
}
#endif

#endif /* XDGLIST_H_ */
