/** @file xdgapp.h
 *  @brief Public file.
 *
 * Data structures for storing \a ".desktop" and \a ".list" files contents.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Specification:
 * @n http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
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

#ifndef __XDG_APP_H_
#define __XDG_APP_H_

#ifndef HAVE_MMAP
#	error Building exdgmime without MMAP is not yet supported.
#endif

#include "xdglist.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct XdgApp          XdgApp;
typedef struct XdgAppGroup     XdgAppGroup;

/**
 * Rebuilds cache which contains information from all
 * \a ".desktop" and \a ".list" files. By default cache
 * is located in "/usr/share/applications/applications.cache".
 *
 * @return a \c errno value if there was an error, otherwise 0.
 */
int xdg_app_rebuild_cache();

/**
 * Checks that cache is valid and exist.
 * By default cache is located in
 * "/usr/share/applications/applications.cache".
 *
 * @return \c TRUE if cache is valid, \c FALSE otherwise.
 */
int xdg_app_cache_is_valid();

/**
 * Looks in \a "Default Applications" section of all \a ".list"
 * files for association of a given \p "mimeType" with
 * \a ".desktop" files.
 *
 * @param mimeType name of the mime type.
 * @return a \c const pointer to XdgList of XdgApp elements
 * or \c NULL if there is no corresponding \a ".desktop" files.
 */
const XdgList *xdg_default_apps_lookup(const char *mimeType);

/**
 * Looks in \a "Added Associations" section of all \a ".list"
 * files for association of a given \p "mimeType" with
 * \a ".desktop" files.
 *
 * @param mimeType name of the mime type.
 * @return a \c const pointer to XdgList of XdgApp elements
 * or \c NULL if there is no corresponding \a ".desktop" files.
 */
const XdgList *xdg_added_apps_lookup(const char *mimeType);

/**
 * Looks in \a "Removed Associations" section of all \a ".list"
 * files for association of a given \p "mimeType" with
 * \a ".desktop" files.
 *
 * @param mimeType name of the mime type.
 * @return a \c const pointer to XdgList of XdgApp elements
 * or \c NULL if there is no corresponding \a ".desktop" files.
 */
const XdgList *xdg_removed_apps_lookup(const char *mimeType);

/**
 * Looks in all \a ".desktop" files for association of
 * a given \p "mimeType" with \a ".desktop" files.
 *
 * @param mimeType name of the mime type.
 * @return a \c const pointer to XdgList of XdgApp elements
 * or \c NULL if there is no corresponding \a ".desktop" files.
 */
const XdgList *xdg_known_apps_lookup(const char *mimeType);

/**
 * Takes an icon name from "Icon" entry of a given \p "app"
 * (\a ".*desktop" file) and searches the icon in a given
 * \p "theme" with a given \p "size".
 *
 * @param app a \c const pointer to XdgApp.
 * @param theme name of the theme.
 * @param size size of the icon.
 * @return a path to the icon if it was found or \c NULL otherwise.
 *
 * \note Result of this function must be freed by the caller!
 */
char *xdg_app_icon_lookup(const XdgApp *app, const char *theme, int size);

/**
 * Searches for a \p "group" (for example \a "Desktop Entry") in
 * a given \p "app" (\a ".*desktop" file).
 *
 * @param app a \c const pointer to XdgApp.
 * @param group name of the group.
 * @return a \c const pointer to XdgAppGroup or \c NULL if there is no
 * corresponding \p "group".
 */
const XdgAppGroup *xdg_app_group_lookup(const XdgApp *app, const char *group);

/**
 * Searches for an \p "entry" (for example \a "Icon") in a given \p "group"
 * (for example \a "Desktop Entry").
 *
 * @param group a \c const pointer to XdgAppGroup.
 * @param entry name of the entry.
 * @return a \c const pointer to XdgList of \c "const char *" values
 * or \c NULL if there is no corresponding \p "entry".
 */
const XdgList *xdg_app_entry_lookup(const XdgAppGroup *group, const char *entry);

/**
 * Searches for an localized \p "entry" (for example \a "Name[ru]") in a given \p "group"
 * (for example \a "Desktop Entry").
 *
 * @param group a \c const pointer to XdgAppGroup.
 * @param entry name of the entry.
 * @param lang language of the localized value.
 * @param country country of the localized value.
 * @param modifier modifier of the localized value.
 * @return a \c const pointer to XdgList of \c XdgEncodedValue values
 * or \c NULL if there is no corresponding \p "entry".
 */
const XdgList *xdg_app_localized_entry_lookup(
		const XdgAppGroup *group,
		const char *entry,
		const char *lang,
		const char *country,
		const char *modifier);

/**
 * Get XdgApp item from current list item.
 *
 * @note This function works for XdgList returned by
 * xdg_default_apps_lookup(), xdg_added_apps_lookup(),
 * xdg_removed_apps_lookup(), xdg_known_apps_lookup() functions.
 *
 * @param list current list item.
 * @return a \c const pointer to XdgApp.
 */
const XdgApp *xdg_list_item_app(const XdgList *list);

/**
 * Get \c "const char *" value from current list item.
 *
 * @note This function works for XdgList returned by
 * xdg_app_entry_lookup(), xdg_app_localized_entry_lookup() functions.
 *
 * @param list current list item.
 * @return a \c const pointer to XdgApp.
 */
const char *xdg_list_item_app_group_entry_value(const XdgList *list);

#ifdef __cplusplus
}
#endif

#endif /* __XDG_APP_H_ */
