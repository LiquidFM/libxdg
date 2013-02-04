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

#include "../containers/xdglist.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct XdgApp      XdgApp;
typedef struct XdgAppGroup XdgAppGroup;
typedef struct XdgUserApps XdgUserApps;


/**
 * This enum determines section in \a ".list" files.
 */
enum ListFileSections
{
    XdgAddedApplications,
    XdgDefaultApplications,
    XdgRemovedApplications
};
typedef enum ListFileSections ListFileSections;


/**
 * Represents result
 * of xdg_app_rebuild_cache_in_each_data_dir() function call.
 */
struct RebuildCacheResult
{
	/**
	 * Value of \c errno.
	 */
	int error;

	/**
	 * Absolute path to directory.
	 */
	char *directory;
};
typedef struct RebuildCacheResult RebuildCacheResult;

/**
 * Represents result
 * of xdg_app_refresh() function call.
 */
struct RebuildResult
{
	/**
	 * Value of \c errno.
	 */
	int error;

	/**
	 * Absolute path to directory.
	 */
	const char *directory;
};
typedef struct RebuildResult RebuildResult;

/**
 * Checks that cache file is valid (all data is up-to-date).
 *
 * @param directory absolute path to directory which
 * contains the cache.
 * @return \c TRUE if cache is valid, \c FALSE otherwise.
 *
 * @note
 * Initialization of the library is not necessary
 * before call to this function.
 */
int xdg_app_cache_file_is_valid(const char *directory);

/**
 * Rebuilds cache in the given \p directory. This cache
 * contains information from all \a ".desktop" and \a ".list"
 * files found in the given \p directory.
 *
 * @param directory absolute path to directory which
 * contains \a ".desktop" and \a ".list" files.
 * @return a \c errno value if there was an error, otherwise 0.
 *
 * @note
 * Initialization of the library is not necessary
 * before call to this function.
 */
int xdg_app_rebuild_cache_file(const char *directory);

/**
 * Rebuilds cache in each directory from \a "XDG_DATA_DIRS"
 * and \a "XDG_DATA_HOME". This cache contains information from
 * all \a ".desktop" and \a ".list" files found in the given
 * directories.
 *
 * @param result a \c pointer to RebuildCacheResult structure
 * which contains result of this function call.
 *
 * @note
 * Caller of this function is responsible for freeing resources
 * of RebuildCacheResult structure.
 * @n Initialization of the library is not necessary
 * before call to this function.
 */
void xdg_app_rebuild_cache_in_each_data_dir(RebuildCacheResult *result);

/**
 * Checks that dynamically loaded data and cache files are valid
 * and reloads it if not.
 *
 * @note This function is not thread safe!
 */
void xdg_app_refresh(RebuildResult *result);

/**
 * Looks for applications able to handle given mime type,
 * according to the information from the merged \a ".list" files.
 *
 * @param mimeType name of the mime type.
 * @return a \c "const pointer" to XdgJointList of XdgApp elements
 * or \c NULL if there is no corresponding \a ".desktop" files.
 */
const XdgJointListItem *xdg_apps_lookup(const char *mimeType);

/**
 * Looks in all \a ".desktop" files for association of
 * a given \p "mimeType" with \a ".desktop" files.
 *
 * @param mimeType name of the mime type.
 * @return a \c "const pointer" to XdgJointList of XdgApp elements
 * or \c NULL if there is no corresponding \a ".desktop" files.
 */
const XdgJointListItem *xdg_known_apps_lookup(const char *mimeType);

XdgUserApps *xdg_user_defined_apps_load();

XdgJointListItem *xdg_user_defined_apps_lookup(XdgUserApps *apps, const char *mimeType, ListFileSections listFileSection);

void xdg_user_defined_apps_free(XdgUserApps *apps, int save);

/**
 * Takes an icon name from "Icon" entry of a given \p "app"
 * (\a ".*desktop" file) and searches the icon in a given
 * \p "theme" with a given \p "size".
 *
 * @param app a \c "const pointer" to XdgApp.
 * @param theme name of the theme.
 * @param size size of the icon.
 * @return a path to the icon if it was found or \c NULL otherwise.
 *
 * @note This function returns \c NULL if  \c THEMES_SPEC was not
 * defined. @n Result of this function must be freed by the caller!
 */
char *xdg_app_icon_lookup(const XdgApp *app, const char *theme, int size);

/**
 * Searches for a \p "group" (for example \a "Desktop Entry") in
 * a given \p "app" (\a ".*desktop" file).
 *
 * @param app a \c "const pointer" to XdgApp.
 * @param group name of the group.
 * @return a \c "const pointer" to XdgAppGroupEntries or \c NULL if there is no
 * corresponding \p "group".
 */
const XdgAppGroup *xdg_app_group_lookup(const XdgApp *app, const char *group);

/**
 * Searches for an \p "entry" (for example \a "Icon") in a given \p "group"
 * (for example \a "Desktop Entry").
 *
 * @param group a \c "const pointer" to XdgAppGroupEntries.
 * @param entry name of the entry.
 * @return a \c "const pointer" to XdgList of default \p entry
 * \c "const char *" values or \c NULL if there is no corresponding \p "entry".
 */
const XdgListItem *xdg_app_entry_lookup(const XdgAppGroup *group, const char *entry);

/**
 * Searches for a localized \p "entry" (for example \a "Name[ru]") in a given \p "group"
 * (for example \a "Desktop Entry").
 *
 * @param group a \c "const pointer" to XdgAppGroupEntries.
 * @param entry name of the entry.
 * @param lang language of the localized value.
 * @param country country of the localized value.
 * @param modifier modifier of the localized value.
 * @return a \c "const pointer" to XdgList of \c XdgEncodedValue values
 * or \c NULL if there is no corresponding \p "entry".
 *
 * @note
 * If there is no corresponding localized value then it returns a
 * \c "const pointer" to XdgList of \a default \c "const char *" values (if any).
 */
const XdgListItem *xdg_app_localized_entry_lookup(
		const XdgAppGroup *group,
		const char *entry,
		const char *lang,
		const char *country,
		const char *modifier);

/**
 * Get XdgApp item from a given \p list item.
 *
 * @note This function works for XdgJointList returned by this functions:
 * @li xdg_apps_lookup()
 * @li xdg_known_apps_lookup()
 *
 * @param list current list item.
 * @return a \c "const pointer" to XdgApp.
 */
const XdgApp *xdg_joint_list_item_app(const XdgJointListItem *list);

/**
 * Get XdgApp item id (".desktop" file name) from a given \p list item.
 *
 * @note This function works for XdgJointList returned by this functions:
 * @li xdg_apps_lookup()
 * @li xdg_known_apps_lookup()
 *
 * @param list current list item.
 * @return a \c "const pointer" to XdgApp.
 */
const char *xdg_joint_list_item_app_id(const XdgJointListItem *list);

/**
 * Get \c "const char *" value from current list item.
 *
 * @note This function works for XdgList returned by
 * xdg_app_entry_lookup(), xdg_app_localized_entry_lookup() functions.
 *
 * @param list current list item.
 * @return s \c "const char *" to value of ".desktop" file group entry.
 */
const char *xdg_list_item_app_group_entry_value(const XdgListItem *list);

#ifdef __cplusplus
}
#endif

#endif /* __XDG_APP_H_ */
