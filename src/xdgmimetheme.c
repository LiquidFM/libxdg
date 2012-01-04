/* xdgmimetheme.c: Private file.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Copyright (C) 2011  Dmitriy Vilkov <dav.daemon@gmail.com>
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

#include "xdgmimetheme_p.h"
#include "xdgmimearray_p.h"
#include "xdgmimedefs.h"
#include "avltree.h"
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <limits.h>
#include <sys/stat.h>


#define READ_FROM_FILE_BUFFER_SIZE 2048
typedef unsigned char BOOL;
#define TRUE 1
#define FALSE 0


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
struct XdgThemeGroupEntry
{
	XdgArray values;
};
typedef struct XdgThemeGroupEntry XdgThemeGroupEntry;

struct XdgThemeGroup
{
	AvlTree entries;
};

struct XdgTheme
{
	AvlTree groups;
	XdgArray parents;
	char name[1];
};


/**
 * Main data structure
 */
struct XdgThemes
{
	AvlTree themes_files_map;
};


/**
 * Function types
 *
 */
typedef void (*XdgIconsScanFunc)(char *buffer, XdgThemes *applications, const char *directory);
typedef char *(*XdgIconsSearchFunc)(XdgTheme *theme, const char *icon, const char **subdirs, const char *directory);


/**
 * Memory allocation functions
 *
 */
static void _xdg_array_string_item_add(XdgArray *array, const char *name)
{
	(*_xdg_array_item_add(array, 2)) = strdup(name);
}

static void _xdg_array_theme_item_add(XdgArray *array, XdgTheme *theme)
{
	(*_xdg_array_item_add(array, 1)) = theme;
}

static XdgThemeGroupEntry *_xdg_theme_group_entry_map_item_add(AvlTree *map, const char *name)
{
	XdgThemeGroupEntry **res = (XdgThemeGroupEntry **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgThemeGroupEntry));
		memset(&(*res)->values, 0, sizeof(XdgArray));
	}

	return (*res);
}

static void _xdg_theme_group_entry_map_item_free(XdgThemeGroupEntry *item)
{
	_xdg_array_and_values_free(&item->values);
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
		memset(&(*res)->parents, 0, sizeof(XdgArray));

		memcpy((*res)->name, name, len);
		(*res)->name[len] = 0;
	}

	return (*res);
}

static void _xdg_theme_map_item_free(XdgTheme *item)
{
	clear_avl_tree_and_values(&item->groups, (DestroyValue)_xdg_theme_group_map_item_free);
	_xdg_array_free(&item->parents);
	free(item);
}


/**
 * Main algorithms
 *
 * TODO: Group theme "Directories" by "Context".
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
					_xdg_array_string_item_add(&entry->values, start);
					_xdg_array_theme_item_add(&theme->parents, _xdg_theme_map_item_add(&themes->themes_files_map, start));
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_array_string_item_add(&entry->values, start);
				_xdg_array_theme_item_add(&theme->parents, _xdg_theme_map_item_add(&themes->themes_files_map, start));
			}
		}
		else
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ',')
				{
					*sep = 0;
					_xdg_array_string_item_add(&entry->values, start);
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_array_string_item_add(&entry->values, start);
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

static void ___xdg_mime_themes_read_from_directory(char *buffer, XdgThemes *themes, const char *directory_name, const char *name)
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

static void __xdg_mime_themes_read_from_directory(char *buffer, XdgThemes *themes, const char *directory_name)
{
	DIR *dir;

	if (dir = opendir(directory_name))
	{
		char *file_name;
		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL)
			if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
			{
				file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
				strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

				___xdg_mime_themes_read_from_directory(buffer, themes, file_name, entry->d_name);

				free(file_name);
			}

		closedir(dir);
	}
}

static void _xdg_run_command_on_icons_dirs(char *buffer, XdgThemes *themes, XdgIconsScanFunc func)
{
	const char *xdg_data_home;
	const char *xdg_data_dirs;
	const char *ptr;

	if ((xdg_data_home = getenv("XDG_DATA_HOME")))
		func(buffer, themes, xdg_data_home);
	else
    {
		const char *home = getenv("HOME");

		if (home != NULL)
		{
			char *guessed_xdg_home;

			guessed_xdg_home = malloc(strlen(home) + strlen("/.icons/") + 1);
			strcpy(guessed_xdg_home, home);
			strcat(guessed_xdg_home, "/.icons/");
			func(buffer, themes, guessed_xdg_home);
			free(guessed_xdg_home);
		}
    }

	xdg_data_dirs = getenv("XDG_DATA_DIRS");
	if (xdg_data_dirs == NULL)
		xdg_data_dirs = "/usr/local/share/:/usr/share/";

	ptr = xdg_data_dirs;

	while (*ptr != '\000')
	{
		const char *end_ptr;
		char *dir;
		int len;

		end_ptr = ptr;
		while (*end_ptr != ':' && *end_ptr != '\000')
			end_ptr++;

		if (end_ptr == ptr)
		{
			ptr++;
			continue;
		}

		if (*end_ptr == ':')
			len = end_ptr - ptr;
		else
			len = end_ptr - ptr + 1;

		dir = malloc(len + strlen("/icons/") + 1);
		strncpy(dir, ptr, len);
		dir[len] = '\0';
		strcat(dir, "/icons/");
		dir[len + strlen("/icons/")] = '\0';
		func(buffer, themes, dir);
		free(dir);

		ptr = end_ptr;
	}

	func(buffer, themes, "/usr/share/pixmaps/");
}

XdgThemes *_xdg_mime_themes_new(void)
{
	XdgThemes *list;

	list = malloc(sizeof(XdgThemes));
	init_avl_tree(&list->themes_files_map, strdup, (DestroyKey)free, strcmp);

	return list;
}

void _xdg_mime_themes_read_from_directory(XdgThemes *themes)
{
	char buffer[READ_FROM_FILE_BUFFER_SIZE];

	_xdg_run_command_on_icons_dirs(buffer, themes, __xdg_mime_themes_read_from_directory);
}

void _xdg_mime_themes_free(XdgThemes *themes)
{
	clear_avl_tree_and_values(&themes->themes_files_map, (DestroyValue)_xdg_theme_map_item_free);
	free(themes);
}

static char *_xdg_run_command_on_icons_dirs_2(XdgTheme *theme, const char *icon, const char **subdirs, XdgIconsSearchFunc func)
{
	const char *xdg_data_home;
	const char *xdg_data_dirs;
	const char *ptr;
	char *res;

	if ((xdg_data_home = getenv("XDG_DATA_HOME")))
	{
		if (res = func(theme, icon, subdirs, xdg_data_home))
			return res;
	}
	else
    {
		const char *home = getenv("HOME");

		if (home != NULL)
		{
			char *guessed_xdg_home;

			guessed_xdg_home = malloc(strlen(home) + strlen("/.icons/") + 1);
			strcpy(guessed_xdg_home, home);
			strcat(guessed_xdg_home, "/.icons/");
			res = func(theme, icon, subdirs, guessed_xdg_home);
			free(guessed_xdg_home);

			if (res)
				return res;
		}
    }

	xdg_data_dirs = getenv("XDG_DATA_DIRS");
	if (xdg_data_dirs == NULL)
		xdg_data_dirs = "/usr/local/share/:/usr/share/";

	ptr = xdg_data_dirs;

	while (*ptr != '\000')
	{
		const char *end_ptr;
		char *dir;
		int len;

		end_ptr = ptr;
		while (*end_ptr != ':' && *end_ptr != '\000')
			end_ptr++;

		if (end_ptr == ptr)
		{
			ptr++;
			continue;
		}

		if (*end_ptr == ':')
			len = end_ptr - ptr;
		else
			len = end_ptr - ptr + 1;

		dir = malloc(len + strlen("/icons/") + 1);
		strncpy(dir, ptr, len);
		dir[len] = '\0';
		strcat(dir, "/icons/");
		dir[len + strlen("/icons/")] = '\0';
		res = func(theme, icon, subdirs, dir);
		free(dir);

		if (res)
			return res;

		ptr = end_ptr;
	}

	if (res = func(theme, icon, subdirs, "/usr/share/pixmaps/"))
		return res;

	return 0;
}

static char *_xdg_mime_search_icon_file(XdgTheme *theme, const char *icon, const char **subdirs, const char *directory)
{
	struct stat buf;
	char *res = NULL;
	char *theme_dir = malloc(strlen(directory) + strlen(theme->name) + 2);
	strcpy(theme_dir, directory); strcat(theme_dir, "/"); strcat(theme_dir, theme->name);

	if (stat(theme_dir, &buf) == 0)
	{
		char *sub_dir;
		char *file_name;
		const char **extensions;

		for (; *subdirs && res == NULL; ++subdirs)
		{
			sub_dir = malloc(strlen(theme_dir) + strlen(*subdirs) + 2);
			strcpy(sub_dir, theme_dir); strcat(sub_dir, "/"); strcat(sub_dir, *subdirs);

			for (extensions = icon_extensions; *extensions; ++extensions)
			{
				file_name = malloc(strlen(sub_dir) + strlen(icon) + strlen(*extensions) + 3);
				strcpy(file_name, sub_dir); strcat(file_name, "/");
				strcat(file_name, icon); strcat(file_name, "."); strcat(file_name, *extensions);

				if (stat(file_name, &buf) == 0)
				{
					res = file_name;
					break;
				}

				free(file_name);
			}

			free(sub_dir);
		}
	}

	free(theme_dir);
	return res;
}

static BOOL _xdg_mime_directory_matches_size(XdgThemeGroup *dirGroup, int size, const char *directory, const char **closestDirName, int *minimal_size)
{
	XdgThemeGroupEntry **entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "Type");

	if (entry1 && (*entry1)->values.count)
	{
		const char *type = (*entry1)->values.list[0];
		entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "Size");

		if (entry1 && (*entry1)->values.count)
		{
			int dirSize = atoi((*entry1)->values.list[0]);

			if (strcmp(type, "Threshold") == 0)
			{
				int threshold = 2;
				entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "Threshold");

				if (entry1 && (*entry1)->values.count)
					threshold = atoi((*entry1)->values.list[0]);

				if (size >= dirSize - threshold && size <= dirSize + threshold)
					return TRUE;
				else
				{
					int tmp_size;

				    if (size < dirSize - threshold)
				    {
						int minSize = dirSize;
						entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "MinSize");

						if (entry1 && (*entry1)->values.count)
							minSize = atoi((*entry1)->values.list[0]);

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

							if (entry1 && (*entry1)->values.count)
								maxSize = atoi((*entry1)->values.list[0]);

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
					if (entry1 && (*entry1)->values.count)
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

						if (entry1 && (*entry1)->values.count)
							minSize = atoi((*entry1)->values.list[0]);

						entry1 = (XdgThemeGroupEntry **)search_node(&dirGroup->entries, "MaxSize");

						if (entry1 && (*entry1)->values.count)
							maxSize = atoi((*entry1)->values.list[0]);

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

	if (entry1 && (*entry1)->values.count &&
		strcmp((*entry1)->values.list[0], contextes[context]) == 0)
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

		if (entry && (*entry)->values.count)
		{
			XdgThemeGroup **dirGroup;
			int minimal_size = INT_MAX;

			int dirs_array_size = sizeof(const char *) * ((*entry)->values.count + 1);
			const char **directories = malloc(dirs_array_size);
			memset(directories, 0, dirs_array_size);

			const char *closestDirName = 0;
			const char **ptr = directories;
			char *dir;

			int i = 0, count = (*entry)->values.count;
			for (; i < count; ++i)
				if ((dirGroup = (XdgThemeGroup **)search_node(&theme->groups, dir = (*entry)->values.list[i])) &&
					_xdg_mime_directory_matches_size_and_context(*dirGroup, size, context, dir, &closestDirName, &minimal_size))
				{
					(*ptr++) = dir;
				}

			if (*directories)
				if (dir = _xdg_run_command_on_icons_dirs_2(theme, icon, directories, _xdg_mime_search_icon_file))
					res = dir;
				else
					if (closestDirName)
					{
						directories[0] = closestDirName;
						directories[1] = NULL;

						if (dir = _xdg_run_command_on_icons_dirs_2(theme, icon, directories, _xdg_mime_search_icon_file))
							res = dir;
					}

			free(directories);
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
		if (theme->parents.count)
		{
			int i = 0, count = theme->parents.count;

			for (; i < count; ++i)
				if (res = _xdg_mime_find_icon_helper(icon, size, context, theme->parents.list[i], hicolor))
					return res;
		}
		else
			if (theme != hicolor)
				return _xdg_mime_find_icon_helper(icon, size, context, hicolor, hicolor);

	return 0;
}

static char *_xdg_mime_lookup_fallback_icon(const char *icon, int size, XdgTheme *theme)
{
	return 0;
}

static char *_xdg_mime_find_icon(const char *icon, int size, Context context, XdgTheme *theme, XdgTheme *hicolor)
{
	char *res = _xdg_mime_find_icon_helper(icon, size, context, theme, hicolor);

	if (res)
		return res;
	else
		return _xdg_mime_lookup_fallback_icon(icon, size, theme);
}

char *_xdg_mime_icon_lookup(XdgThemes *themes, const char *icon, int size, Context context, const char *themeName)
{
	XdgTheme **hicolor = (XdgTheme **)search_node(&themes->themes_files_map, "hicolor");

	if (hicolor)
	{
		XdgTheme **theme = (XdgTheme **)search_node(&themes->themes_files_map, themeName);

		if (theme)
			return _xdg_mime_find_icon(icon, size, context, *theme, *hicolor);
	}

	return 0;
}
