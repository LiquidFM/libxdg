/* xdgtheme.c: Private file.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Copyright (C) 2011,2012  Dmitriy Vilkov <dav.daemon@gmail.com>
 *
 * Licensed under the Academic Free License version 2.0
 * Or under the following terms:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "xdgtheme_p.h"
#include "../desktop/xdgmimedefs.h"
#include "../containers/avltree_p.h"
#include "../containers/xdglist_p.h"
#include "../basedirectory/xdgbasedirectory.h"
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <limits.h>
#include <sys/stat.h>


static const char *contextes[] =
{
	"Actions",
	"Animations",
	"Applications",
	"Categories",
	"Devices",
	"Emblems",
	"Emotes",
	"FileSystems",
	"International",
	"MimeTypes",
	"Places",
	"Status",
	"Stock"
};
static const char *icon_extensions[] = {"png", "svg", "xpm", NULL};


/**
 * ".theme" files data
 */
struct XdgStringList
{
	XdgList list;
	char value[1];
};
typedef struct XdgStringList XdgStringList;

struct XdgThemeGroupEntry
{
	XdgStringList *values;
};
typedef struct XdgThemeGroupEntry XdgThemeGroupEntry;

struct XdgThemeGroup
{
	AvlTree entries;
};

struct XdgThemeParents
{
	XdgList list;
	XdgTheme *parent;
};
typedef struct XdgThemeParents XdgThemeParents;

struct XdgTheme
{
	AvlTree groups;
	XdgThemeParents *parents;
	char name[1];
};


/**
 * Main data structure
 */
struct XdgIconSearchFuncArgs
{
	XdgTheme *theme;
	const char *icon;
	XdgStringList *subdirs;
};
typedef struct XdgIconSearchFuncArgs XdgIconSearchFuncArgs;


struct XdgThemes
{
	AvlTree themes_files_map;
};


typedef struct XdgThemes XdgThemes;
static XdgThemes *themes_list = NULL;


/**
 * Memory allocation functions
 *
 */
static void _xdg_list_string_item_add(XdgStringList **list, const char *name)
{
	XdgStringList *res = malloc(sizeof(XdgStringList) + strlen(name));

	_xdg_list_apped((XdgList **)list, (XdgList *)res);

	strcpy(res->value, name);
}

static void _xdg_list_string_item_copy(XdgStringList **list, XdgStringList *item)
{
	XdgStringList *res = malloc(sizeof(XdgStringList) + strlen(item->value));

	_xdg_list_apped((XdgList **)list, (XdgList *)res);

	strcpy(res->value, item->value);
}

static void _xdg_list_theme_item_add(XdgThemeParents **list, XdgTheme *theme)
{
	XdgThemeParents *res = malloc(sizeof(XdgThemeParents));

	_xdg_list_apped((XdgList **)list, (XdgList *)res);

	res->parent = theme;
}

static XdgThemeGroupEntry *_xdg_theme_group_entry_map_item_add(AvlTree *map, const char *name)
{
	XdgThemeGroupEntry **res = (XdgThemeGroupEntry **)search_or_create_node(map, name);

	if ((*res) == NULL)
		(*res) = calloc(1, sizeof(XdgThemeGroupEntry));

	return (*res);
}

static void _xdg_theme_group_entry_map_item_free(XdgThemeGroupEntry *item)
{
	_xdg_list_free((XdgList *)item->values, free);
	free(item);
}

static XdgThemeGroup *_xdg_theme_group_map_item_add(AvlTree *map, const char *name)
{
	XdgThemeGroup **res = (XdgThemeGroup **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgThemeGroup));
		init_avl_tree(&(*res)->entries, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_theme_group_map_item_free(XdgThemeGroup *item)
{
	clear_avl_tree_and_values(&item->entries, (DestroyValue)_xdg_theme_group_entry_map_item_free);
	free(item);
}

static XdgTheme *_xdg_theme_map_item_add(AvlTree *map, const char *name)
{
	XdgTheme **res = (XdgTheme **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		int len = strlen(name);
		(*res) = malloc(sizeof(XdgTheme) + len);

		init_avl_tree(&(*res)->groups, strdup, (DestroyKey)free, strcmp);
		(*res)->parents = NULL;

		memcpy((*res)->name, name, len);
		(*res)->name[len] = 0;
	}

	return (*res);
}

static void _xdg_theme_map_item_free(XdgTheme *item)
{
	clear_avl_tree_and_values(&item->groups, (DestroyValue)_xdg_theme_group_map_item_free);
	_xdg_list_free((XdgList *)item->parents, free);
	free(item);
}


/**
 * Main algorithms
 *
 * TODO: Index "Directories" by "Context".
 */
static void _xdg_theme_group_read_entry(XdgThemes *themes, XdgTheme *theme, XdgThemeGroup *group, const char *line)
{
	char *sep;

	if ((sep = strchr(line, '=')) != NULL)
	{
		char *start = sep + 1;
		REMOVE_WHITE_SPACES_LEFT(line, sep)
		REMOVE_WHITE_SPACES_RIGHT(start)
		XdgThemeGroupEntry *entry = _xdg_theme_group_entry_map_item_add(&group->entries, line);

		if (strcmp(line, "Inherits") == 0)
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ',')
				{
					*sep = 0;
					_xdg_list_string_item_add(&entry->values, start);
					_xdg_list_theme_item_add(&theme->parents, _xdg_theme_map_item_add(&themes->themes_files_map, start));
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_list_string_item_add(&entry->values, start);
				_xdg_list_theme_item_add(&theme->parents, _xdg_theme_map_item_add(&themes->themes_files_map, start));
			}
		}
		else
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ',')
				{
					*sep = 0;
					_xdg_list_string_item_add(&entry->values, start);
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_list_string_item_add(&entry->values, start);
			}
		}
	}
}

static void _xdg_applications_read_theme_file(char *buffer, XdgThemes *themes, FILE *file, const char *name)
{
	char *sep;
	XdgTheme *theme;
	XdgThemeGroup *group = NULL;

	theme = _xdg_theme_map_item_add(&themes->themes_files_map, name);

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n')
			if (buffer[0] == '[')
			{
				group = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
				{
					*sep = 0;
					group = _xdg_theme_group_map_item_add(&theme->groups, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_theme_group_read_entry(themes, theme, group, buffer);
				else
					break;
}

static void __xdg_mime_themes_read_from_directory(char *buffer, XdgThemes *themes, const char *directory_name, const char *name)
{
	DIR *dir;

	if (dir = opendir(directory_name))
	{
		FILE *file;
		char *file_name;
		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL)
			if (entry->d_type == DT_REG && strcmp(entry->d_name, "index.theme") == 0)
			{
				file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
				strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

				if (file = fopen(file_name, "r"))
				{
					_xdg_applications_read_theme_file(buffer, themes, file, name);
					fclose(file);
				}

				free(file_name);
				break;
			}

		closedir(dir);
	}
}

static void _xdg_mime_themes_read_from_directory(const char *directory, char *buffer)
{
	DIR *dir;

	if (dir = opendir(directory))
	{
		char *file_name;
		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL)
			if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
			{
				file_name = malloc(strlen(directory) + strlen(entry->d_name) + 2);
				strcpy(file_name, directory); strcat(file_name, "/"); strcat(file_name, entry->d_name);

				__xdg_mime_themes_read_from_directory(buffer, themes_list, file_name, entry->d_name);

				free(file_name);
			}

		closedir(dir);
	}
}

static XdgThemes *_xdg_mime_themes_new(void)
{
	XdgThemes *res;

	res = malloc(sizeof(XdgThemes));
	init_avl_tree(&res->themes_files_map, strdup, (DestroyKey)free, strcmp);

	return res;
}

static void _xdg_mime_themes_free(XdgThemes *themes)
{
	clear_avl_tree_and_values(&themes->themes_files_map, (DestroyValue)_xdg_theme_map_item_free);
	free(themes);
}

void _xdg_themes_init()
{
	char buffer[READ_FROM_FILE_BUFFER_SIZE];

	themes_list = _xdg_mime_themes_new();
	_xdg_for_each_theme_dir((XdgDirectoryFunc)_xdg_mime_themes_read_from_directory, buffer);
}

void _xdg_themes_shutdown()
{
	if (themes_list)
	{
		_xdg_mime_themes_free(themes_list);
		themes_list = NULL;
	}
}

static char *_xdg_search_icon_file(const char *directory, XdgIconSearchFuncArgs *data)
{
	struct stat buf;
	char *res = NULL;
	char *theme_dir = malloc(strlen(directory) + strlen(data->theme->name) + 2);
	strcpy(theme_dir, directory); strcat(theme_dir, "/"); strcat(theme_dir, data->theme->name);

	if (stat(theme_dir, &buf) == 0)
	{
		char *sub_dir;
		char *file_name;
		const char **extensions;
		XdgStringList *subdir = (XdgStringList *)data->subdirs->list.head;

		do
		{
			sub_dir = malloc(strlen(theme_dir) + strlen(subdir->value) + 2);
			strcpy(sub_dir, theme_dir); strcat(sub_dir, "/"); strcat(sub_dir, subdir->value);

			for (extensions = icon_extensions; *extensions; ++extensions)
			{
				file_name = malloc(strlen(sub_dir) + strlen(data->icon) + strlen(*extensions) + 3);
				strcpy(file_name, sub_dir); strcat(file_name, "/");
				strcat(file_name, data->icon); strcat(file_name, "."); strcat(file_name, *extensions);

				if (stat(file_name, &buf) == 0)
				{
					res = file_name;
					break;
				}

				free(file_name);
			}

			free(sub_dir);
		}
		while (subdir = (XdgStringList *)subdir->list.next);
	}

	free(theme_dir);
	return res;
}

static BOOL _xdg_mime_directory_matches_size(XdgThemeGroup *dirGroup, int size, const char *directory, const char **closestDirName, int *minimal_size)
{
	XdgThemeGroupEntry **entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "Type");

	if (entry1 && (*entry1)->values)
	{
		const char *type = (*entry1)->values->value;
		entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "Size");

		if (entry1 && (*entry1)->values)
		{
			int dirSize = atoi((*entry1)->values->value);

			if (strcmp(type, "Threshold") == 0)
			{
				int threshold = 2;
				entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "Threshold");

				if (entry1 && (*entry1)->values)
					threshold = atoi((*entry1)->values->value);

				if (size >= dirSize - threshold && size <= dirSize + threshold)
					return TRUE;
				else
				{
					int tmp_size;

				    if (size < dirSize - threshold)
				    {
						int minSize = dirSize;
						entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "MinSize");

						if (entry1 && (*entry1)->values)
							minSize = atoi((*entry1)->values->value);

						if ((tmp_size = minSize - size) < (*minimal_size))
						{
							(*minimal_size) = tmp_size;
							(*closestDirName) = directory;
						}
				    }
				    else
						if (size > dirSize + threshold)
						{
							int maxSize = dirSize;
							entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "MaxSize");

							if (entry1 && (*entry1)->values)
								maxSize = atoi((*entry1)->values->value);

							if ((tmp_size = size - maxSize) < (*minimal_size))
							{
								(*minimal_size) = tmp_size;
								(*closestDirName) = directory;
							}
						}
						else
						{
							(*minimal_size) = 0;
							(*closestDirName) = directory;
						}
				}
			}
			else
				if (strcmp(type, "Fixed") == 0)
				{
					if (entry1 && (*entry1)->values)
						if (size == dirSize)
							return TRUE;
						else
						{
							int tmp_size;

							if ((tmp_size = abs(dirSize - size)) < (*minimal_size))
							{
								(*minimal_size) = tmp_size;
								(*closestDirName) = directory;
							}
						}
				}
				else
					if (strcmp(type, "Scaled") == 0)
					{
						int minSize = dirSize;
						int maxSize = dirSize;
						entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "MinSize");

						if (entry1 && (*entry1)->values)
							minSize = atoi((*entry1)->values->value);

						entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "MaxSize");

						if (entry1 && (*entry1)->values)
							maxSize = atoi((*entry1)->values->value);

						if (size >= minSize && size <= maxSize)
							return TRUE;
						else
						{
							int tmp_size;

						    if (size < minSize)
						    {
						        if ((tmp_size = minSize - size) < (*minimal_size))
						        {
									(*minimal_size) = tmp_size;
									(*closestDirName) = directory;
						        }
						    }
						    else
						    	if (size > maxSize)
						    	{
						    		if ((tmp_size = size - maxSize) < (*minimal_size))
						    		{
										(*minimal_size) = tmp_size;
										(*closestDirName) = directory;
						    		}
						    	}
						    	else
						    	{
									(*minimal_size) = 0;
									(*closestDirName) = directory;
						    	}
						}
					}
			}
	}

	return FALSE;
}

static BOOL _xdg_mime_directory_matches_size_and_context(XdgThemeGroup *dirGroup, int size, Context context, const char *directory, const char **closestDirName, int *minimal_size)
{
	XdgThemeGroupEntry **entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "Context");

	if (entry1 && (*entry1)->values &&
		strcmp((*entry1)->values->value, contextes[context]) == 0)
			return _xdg_mime_directory_matches_size(dirGroup, size, directory, closestDirName, minimal_size);

	return FALSE;
}

static char *_xdg_mime_lookup_icon(const char *icon, int size, Context context, XdgTheme *theme)
{
	char *res = 0;
	XdgThemeGroup **group = (XdgThemeGroup **)search_node(&theme->groups, "Icon Theme");

	if (group)
	{
		XdgThemeGroupEntry **entry = (XdgThemeGroupEntry **)search_node(&(*group)->entries, "Directories");

		if (entry && (*entry)->values)
		{
			XdgThemeGroup **dirGroup;
			int minimal_size = INT_MAX;
			XdgStringList *directories = NULL;
			XdgStringList *directory = (XdgStringList *)(*entry)->values->list.head;

			const char *closestDirName = 0;
			char *dir;

			do
			{
				if ((dirGroup = (XdgThemeGroup **)search_node(&theme->groups, dir = directory->value)) &&
					_xdg_mime_directory_matches_size_and_context(*dirGroup, size, context, dir, &closestDirName, &minimal_size))
				{
					_xdg_list_string_item_copy(&directories, directory);
				}
			}
			while (directory = (XdgStringList *)directory->list.next);

			if (directories)
			{
				XdgIconSearchFuncArgs data = {theme, icon, directories};

				if (dir = _xdg_search_in_each_theme_dir((XdgIconSearchFunc)_xdg_search_icon_file, &data))
					res = dir;
				else
					if (closestDirName)
					{
						XdgStringList *tmp_directories = NULL;

						_xdg_list_string_item_add(&tmp_directories, closestDirName);
						data.subdirs = tmp_directories;

						if (dir = _xdg_search_in_each_theme_dir((XdgIconSearchFunc)_xdg_search_icon_file, &data))
							res = dir;

						_xdg_list_free((XdgList *)tmp_directories, free);
					}
			}

			_xdg_list_free((XdgList *)directories, free);
		}
	}

	return res;
}

static char *_xdg_mime_find_icon_helper(const char *icon, int size, Context context, XdgTheme *theme, XdgTheme *hicolor)
{
	char *res = _xdg_mime_lookup_icon(icon, size, context, theme);

	if (res)
		return res;
	else
		if (theme->parents)
		{
			XdgThemeParents *parent = (XdgThemeParents *)theme->parents->list.head;

			do
				if (res = _xdg_mime_find_icon_helper(icon, size, context, theme->parents->parent, hicolor))
					return res;
			while (parent = (XdgThemeParents *)parent->list.next);
		}
		else
			if (theme != hicolor)
				return _xdg_mime_find_icon_helper(icon, size, context, hicolor, hicolor);

	return NULL;
}

static char *_xdg_mime_lookup_fallback_icon(const char *icon, int size, XdgTheme *theme)
{
	return NULL;
}

static char *_xdg_mime_find_icon(const char *icon, int size, Context context, XdgTheme *theme, XdgTheme *hicolor)
{
	char *res = _xdg_mime_find_icon_helper(icon, size, context, theme, hicolor);

	if (res)
		return res;
	else
		return _xdg_mime_lookup_fallback_icon(icon, size, theme);
}

char *xdg_mime_type_icon_lookup(const char *mime, int size, const char *themeName)
{
	if (mime)
	{
		XdgTheme **hicolor = (XdgTheme **)search_node(&themes_list->themes_files_map, "hicolor");

		if (hicolor)
		{
			XdgTheme **theme = (XdgTheme **)search_node(&themes_list->themes_files_map, themeName);

			if (theme)
			{
				char *sep;
				char mimeTypeCopy[MIME_TYPE_NAME_BUFFER_SIZE];
				strncpy(mimeTypeCopy, mime, MIME_TYPE_NAME_BUFFER_SIZE);

				if ((sep = strchr(mimeTypeCopy, '/')) != NULL)
					(*sep) = '-';

				return _xdg_mime_find_icon(mimeTypeCopy, size, XdgThemeMimeTypes, *theme, *hicolor);
			}
		}
	}

	return NULL;
}

char *xdg_icon_lookup(const char *icon, int size, Context context, const char *themeName)
{
	if (icon)
	{
		XdgTheme **hicolor = (XdgTheme **)search_node(&themes_list->themes_files_map, "hicolor");

		if (hicolor)
		{
			XdgTheme **theme = (XdgTheme **)search_node(&themes_list->themes_files_map, themeName);

			if (theme)
				return _xdg_mime_find_icon(icon, size, context, *theme, *hicolor);
		}
	}

	return NULL;
}
