/* xdgmimeapp.c: Private file.  Datastructure for storing
 * the .desktop files.
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

#include "xdgmimeapp_p.h"
#include "avltree.h"
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>


#define READ_FROM_FILE_BUFFER_SIZE 1024
#define MIME_TYPE_NAME_BUFFER_SIZE 128

#define REMOVE_WHITE_SPACES_LEFT(line, ptr) \
	do { *ptr = 0; --ptr; } while(ptr > line && (*ptr) == ' ');

#define REMOVE_WHITE_SPACES_RIGHT(ptr) \
	while (*ptr == ' ' && *ptr != '\n' && *ptr != 0) ++ptr;

typedef int (*XdgIconsDirectoryFunc)(char *buffer, XdgApplications *applications, const char *directory);

/**
 * Data structures
 *
 */
struct XdgArray
{
	void **list;
	int count;
	int capacity;
};


/**
 * ".desktop" files data
 */
struct XdgAppGroupEntry
{
	XdgArray values;
};
typedef struct XdgAppGroupEntry XdgAppGroupEntry;

struct XdgAppGroup
{
	AvlTree entries;
};

struct XdgApp
{
	AvlTree groups;
};


/**
 * ".list" files data
 */
struct XdgMimeSubType
{
	XdgArray apps;
};
typedef struct XdgMimeSubType XdgMimeSubType;

struct XdgMimeType
{
	AvlTree sub_types;
};
typedef struct XdgMimeType XdgMimeType;

struct XdgMimeGroup
{
	AvlTree types;
};
typedef struct XdgMimeGroup XdgMimeGroup;


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
typedef struct XdgThemeGroup XdgThemeGroup;

struct XdgTheme
{
	AvlTree groups;
	XdgArray parents;
	char name[1];
};
typedef struct XdgTheme XdgTheme;


/**
 * Main data structure
 */
struct XdgApplications
{
	AvlTree app_files_map;
	AvlTree lst_files_map;
	AvlTree asoc_map;
	AvlTree themes_files_map;
};


/**
 * Memory allocation functions
 *
 */
static void **_xdg_array_item_add(XdgArray *array, int alloc_granularity)
{
	void **res;

	if (array->capacity == 0)
		if (array->list == NULL)
		{
			array->list = malloc(alloc_granularity * sizeof(void *));
			array->capacity = alloc_granularity;
		}
		else
		{
			array->list = realloc(array->list, array->count * 2 * sizeof(void *));
			array->capacity = array->count;
		}

	res = &array->list[array->count];
	++array->count;
	--array->capacity;

	return res;
}

static void _xdg_array_string_item_add(XdgArray *array, const char *name)
{
	(*_xdg_array_item_add(array, 2)) = strdup(name);
}

static void _xdg_array_app_item_add(XdgArray *array, XdgApp *app)
{
	(*_xdg_array_item_add(array, 2)) = app;
}

static void _xdg_array_theme_item_add(XdgArray *array, XdgTheme *theme)
{
	(*_xdg_array_item_add(array, 1)) = theme;
}

static void _xdg_array_and_values_free(XdgArray *array)
{
	if (array->list)
	{
		int i = 0, size = array->count;

		for (; i < size; ++i)
			free(array->list[i]);

		free(array->list);
	}
}

static void _xdg_array_free(XdgArray *array)
{
	free(array->list);
}

static XdgAppGroupEntry *_xdg_app_group_entry_map_item_add(AvlTree *map, const char *name)
{
	XdgAppGroupEntry **res = (XdgAppGroupEntry **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgAppGroupEntry));
		memset(&(*res)->values, 0, sizeof(XdgArray));
	}

	return (*res);
}

static void _xdg_app_group_entry_map_item_free(XdgAppGroupEntry *item)
{
	_xdg_array_and_values_free(&item->values);
	free(item);
}

static XdgAppGroup *_xdg_app_group_map_item_add(AvlTree *map, const char *name)
{
	XdgAppGroup **res = (XdgAppGroup **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgAppGroup));
		init_avl_tree(&(*res)->entries, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_app_group_map_item_free(XdgAppGroup *item)
{
	clear_avl_tree_and_values(&item->entries, (DestroyValue)_xdg_app_group_entry_map_item_free);
	free(item);
}

static XdgApp *_xdg_app_map_item_add(AvlTree *map, const char *name)
{
	XdgApp **res = (XdgApp **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgApp));
		init_avl_tree(&(*res)->groups, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_app_map_item_free(XdgApp *item)
{
	clear_avl_tree_and_values(&item->groups, (DestroyValue)_xdg_app_group_map_item_free);
	free(item);
}

static XdgMimeSubType *_xdg_mime_sub_type_map_item_add(AvlTree *map, const char *name)
{
	XdgMimeSubType **res = (XdgMimeSubType **)search_or_create_node(map, name);

	if ((*res) == NULL)
		(*res) = calloc(1, sizeof(XdgMimeSubType));

	return (*res);
}

static void _xdg_mime_sub_type_map_item_free(XdgMimeSubType *sub_type)
{
	_xdg_array_free(&sub_type->apps);
	free(sub_type);
}

static XdgMimeType *_xdg_mime_type_map_item_add(AvlTree *map, const char *name)
{
	XdgMimeType **res = (XdgMimeType **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgMimeType));
		init_avl_tree(&(*res)->sub_types, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_mime_type_map_item_free(XdgMimeType *type)
{
	clear_avl_tree_and_values(&type->sub_types, (DestroyValue)_xdg_mime_sub_type_map_item_free);
	free(type);
}

static XdgMimeGroup *_xdg_mime_group_map_item_add(AvlTree *map, const char *name)
{
	XdgMimeGroup **res = (XdgMimeGroup **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgMimeGroup));
		init_avl_tree(&(*res)->types, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_mime_group_map_item_free(XdgMimeGroup *group)
{
	clear_avl_tree_and_values(&group->types, (DestroyValue)_xdg_mime_type_map_item_free);
	free(group);
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
 */
static XdgMimeSubType *_xdg_mime_sub_type_item_read(AvlTree *map, const char *name)
{
	char *sep;

	if ((sep = strchr(name, '/')) != NULL)
	{
		*sep = 0;
		XdgMimeType *type = _xdg_mime_type_map_item_add(map, name);
		char *start = (++sep);

		if (*start && *start != '\n')
			return _xdg_mime_sub_type_map_item_add(&type->sub_types, start);
	}

	return 0;
}

static XdgMimeSubType *_xdg_mime_sub_type_item_search(AvlTree *map, const char *name)
{
	char *sep;

	if ((sep = strchr(name, '/')) != NULL)
	{
		*sep = 0;
		XdgMimeType **type = (XdgMimeType **)search_node(map, name);

		if (type)
		{
			char *start = (++sep);

			if (*start && *start != '\n')
			{
				XdgMimeSubType **res = (XdgMimeSubType **)search_node(&(*type)->sub_types, start);

				if (res)
					return (*res);
			}
		}
	}

	return 0;
}

static void _xdg_app_group_read_entry(XdgAppGroup *group, XdgApp *app, AvlTree *asoc_map, const char *line)
{
	char *sep;
	XdgMimeSubType *sub_type;

	if ((sep = strchr(line, '=')) != NULL)
	{
		char *start = sep + 1;
		REMOVE_WHITE_SPACES_LEFT(line, sep)
		REMOVE_WHITE_SPACES_RIGHT(start)
		XdgAppGroupEntry *entry = _xdg_app_group_entry_map_item_add(&group->entries, line);

		if (strcmp(line, "MimeType") == 0)
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_array_string_item_add(&entry->values, start);

					if (sub_type = _xdg_mime_sub_type_item_read(asoc_map, start))
						_xdg_array_app_item_add(&sub_type->apps, app);

					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_array_string_item_add(&entry->values, start);

				if (sub_type = _xdg_mime_sub_type_item_read(asoc_map, start))
					_xdg_array_app_item_add(&sub_type->apps, app);
			}
		}
		else
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
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

static void _xdg_mime_group_read_entry(AvlTree *app_files_map, XdgMimeGroup *group, const char *line)
{
	char *sep;

	if ((sep = strchr(line, '=')) != NULL)
	{
		char *start = sep + 1;
		REMOVE_WHITE_SPACES_LEFT(line, sep)
		REMOVE_WHITE_SPACES_RIGHT(start)
		XdgMimeSubType *sub_type = _xdg_mime_sub_type_item_read(&group->types, line);

		if (sub_type)
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_array_app_item_add(&sub_type->apps, _xdg_app_map_item_add(app_files_map, start));
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_array_app_item_add(&sub_type->apps, _xdg_app_map_item_add(app_files_map, start));
			}
		}
	}
}

static void _xdg_theme_group_read_entry(AvlTree *themes_files_map, XdgTheme *theme, XdgThemeGroup *group, const char *line)
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
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_array_string_item_add(&entry->values, start);
					_xdg_array_theme_item_add(&theme->parents, _xdg_theme_map_item_add(themes_files_map, start));
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_array_string_item_add(&entry->values, start);
				_xdg_array_theme_item_add(&theme->parents, _xdg_theme_map_item_add(themes_files_map, start));
			}
		}
		else
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
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

static void _xdg_applications_read_desktop_file(char *buffer, XdgApplications *applications, FILE *file, const char *name)
{
	char *sep;
	XdgApp *app;
	XdgAppGroup *group = NULL;

	app = _xdg_app_map_item_add(&applications->app_files_map, name);

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n')
			if (buffer[0] == '[')
			{
				group = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
				{
					*sep = 0;
					group = _xdg_app_group_map_item_add(&app->groups, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_app_group_read_entry(group, app, &applications->asoc_map, buffer);
				else
					break;
}

static void _xdg_applications_read_list_file(char *buffer, XdgApplications *applications, FILE *file)
{
	char *sep;
	XdgMimeGroup *group = NULL;

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n')
			if (buffer[0] == '[')
			{
				group = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
				{
					*sep = 0;
					group = _xdg_mime_group_map_item_add(&applications->lst_files_map, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_mime_group_read_entry(&applications->app_files_map, group, buffer);
				else
					break;
}

static void _xdg_applications_read_theme_file(char *buffer, XdgApplications *applications, FILE *file, const char *name)
{
	char *sep;
	XdgTheme *theme;
	XdgThemeGroup *group = NULL;

	theme = _xdg_theme_map_item_add(&applications->themes_files_map, name);

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
					_xdg_theme_group_read_entry(&applications->themes_files_map, theme, group, buffer);
				else
					break;
}

static void __xdg_applications_read_from_directory(char *buffer, XdgApplications *applications, const char *directory_name, const char *preffix)
{
	DIR *dir;

	if (dir = opendir(directory_name))
	{
		FILE *file;
		char *file_name;
		char *file_name_preffix;
		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL)
			if (entry->d_type == DT_DIR)
			{
				if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
				{
					file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
					strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

					file_name_preffix = malloc(strlen(preffix) + strlen(entry->d_name) + 2);
					strcpy(file_name_preffix, preffix); strcat(file_name_preffix, entry->d_name); strcat(file_name_preffix, "-");

					__xdg_applications_read_from_directory(buffer, applications, file_name, file_name_preffix);

					free(file_name_preffix);
					free(file_name);
				}
			}
			else
				if (entry->d_type == DT_REG)
					if (fnmatch("*.desktop", entry->d_name, FNM_NOESCAPE) != FNM_NOMATCH)
					{
						file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
						strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

						if (file = fopen(file_name, "r"))
						{
							file_name_preffix = malloc(strlen(preffix) + strlen(entry->d_name) + 1);
							strcpy(file_name_preffix, preffix); strcat(file_name_preffix, entry->d_name);

							_xdg_applications_read_desktop_file(buffer, applications, file, file_name_preffix);

							free(file_name_preffix);
							fclose(file);
						}

						free(file_name);
					}
					else
						if (fnmatch("*.list", entry->d_name, FNM_NOESCAPE) != FNM_NOMATCH)
						{
							file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
							strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

							if (file = fopen(file_name, "r"))
							{
								_xdg_applications_read_list_file(buffer, applications, file);
								fclose(file);
							}

							free(file_name);
						}

		closedir(dir);
	}
}

void _xdg_mime_applications_read_from_directory(XdgApplications *applications, const char *directory_name)
{
	char buffer[READ_FROM_FILE_BUFFER_SIZE];
	const char *file_name_preffix = "";

	__xdg_applications_read_from_directory(buffer, applications, directory_name, file_name_preffix);
}

static void ___xdg_mime_themes_read_from_directory(char *buffer, XdgApplications *applications, const char *directory_name, const char *name)
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
					_xdg_applications_read_theme_file(buffer, applications, file, name);
					fclose(file);
				}

				free(file_name);
			}

		closedir(dir);
	}
}

static int __xdg_mime_themes_read_from_directory(char *buffer, XdgApplications *applications, const char *directory_name)
{
	DIR *dir;

	if (dir = opendir(directory_name))
	{
		char *file_name;
		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL)
			if (entry->d_type == DT_DIR)
			{
				if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
				{
					file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
					strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

					___xdg_mime_themes_read_from_directory(buffer, applications, file_name, entry->d_name);

					free(file_name);
				}
			}

		closedir(dir);
	}

	return 0;
}

static int xdg_run_command_on_icons_dirs(char *buffer, XdgApplications *applications, XdgIconsDirectoryFunc func)
{
	const char *xdg_data_home;
	const char *xdg_data_dirs;
	const char *ptr;

	if ((xdg_data_home = getenv("XDG_DATA_HOME")))
	{
		if (func(buffer, applications, xdg_data_home))
			return 1;
	}
	else
    {
		const char *home = getenv ("HOME");

		if (home != NULL)
		{
			char *guessed_xdg_home;
			int stop_processing;

			guessed_xdg_home = malloc(strlen(home) + strlen("/.icons/") + 1);
			strcpy(guessed_xdg_home, home);
			strcat(guessed_xdg_home, "/.icons/");
			stop_processing = func(buffer, applications, guessed_xdg_home);
			free(guessed_xdg_home);

			if (stop_processing)
				return 1;
		}
    }

	xdg_data_dirs = getenv ("XDG_DATA_DIRS");
	if (xdg_data_dirs == NULL)
		xdg_data_dirs = "/usr/local/share/:/usr/share/";

	ptr = xdg_data_dirs;

	while (*ptr != '\000')
	{
		const char *end_ptr;
		char *dir;
		int len;
		int stop_processing;

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
		stop_processing = func(buffer, applications, dir);
		free(dir);

		if (stop_processing)
			return 1;

		ptr = end_ptr;
	}

	if (func(buffer, applications, "/usr/share/pixmaps/"))
		return 1;

	return 0;
}

void _xdg_mime_themes_read_from_directory(XdgApplications *applications)
{
	char buffer[READ_FROM_FILE_BUFFER_SIZE];

	xdg_run_command_on_icons_dirs(buffer, applications, __xdg_mime_themes_read_from_directory);
}

static const char *_xdg_mime_lookup_icon(const char *icon, int size, XdgTheme *theme)
{
	const char *res ;
}

static const char *_xdg_mime_lookup_fallback_icon(const char *icon, int size, XdgTheme *theme)
{

}

static const char *_xdg_mime_find_icon_helper(const char *icon, int size, XdgTheme *theme, XdgTheme *hicolor)
{
	const char *res = _xdg_mime_lookup_icon(icon, size, theme);

	if (res)
		return res;
	else
		if (theme->parents.count)
		{
			int i = 0, size = theme->parents.count;

			for (; i < size; ++i)
				if (res = _xdg_mime_find_icon_helper(icon, size, theme->parents.list[i], hicolor))
					return res;
		}
		else
			if (theme != hicolor)
				return _xdg_mime_find_icon_helper(icon, size, hicolor, hicolor);

	return 0;
}

static const char *_xdg_mime_find_icon(const char *icon, int size, XdgTheme *theme, XdgTheme *hicolor)
{
	const char *res = _xdg_mime_find_icon_helper(icon, size, theme, hicolor);

	if (res)
		return res;
	else
		return _xdg_mime_lookup_fallback_icon(icon, size, theme);
}

XdgApplications *_xdg_mime_applications_new(void)
{
	XdgApplications *list;

	list = malloc(sizeof(XdgApplications));
	init_avl_tree(&list->app_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&list->lst_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&list->asoc_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&list->themes_files_map, strdup, (DestroyKey)free, strcmp);

	return list;
}

void _xdg_mime_applications_free(XdgApplications *applications)
{
	clear_avl_tree_and_values(&applications->app_files_map, (DestroyValue)_xdg_app_map_item_free);
	clear_avl_tree_and_values(&applications->lst_files_map, (DestroyValue)_xdg_mime_group_map_item_free);
	clear_avl_tree_and_values(&applications->asoc_map, (DestroyValue)_xdg_mime_type_map_item_free);
	clear_avl_tree_and_values(&applications->themes_files_map, (DestroyValue)_xdg_theme_map_item_free);
	free(applications);
}

const XdgArray *_xdg_mime_default_apps_lookup(XdgApplications *applications, const char *mimeType)
{
	XdgMimeGroup **group = (XdgMimeGroup **)search_node(&applications->lst_files_map, "Default Applications");

	if (group)
	{
		char mimeTypeCopy[MIME_TYPE_NAME_BUFFER_SIZE];
		strncpy(mimeTypeCopy, mimeType, MIME_TYPE_NAME_BUFFER_SIZE);
		XdgMimeSubType *sub_type = _xdg_mime_sub_type_item_search(&(*group)->types, mimeTypeCopy);

		if (sub_type)
			return &sub_type->apps;
	}

	return 0;
}

const XdgArray *_xdg_mime_user_apps_lookup(XdgApplications *applications, const char *mimeType)
{
	XdgMimeGroup **group = (XdgMimeGroup **)search_node(&applications->lst_files_map, "Added Associations");

	if (group)
	{
		char mimeTypeCopy[MIME_TYPE_NAME_BUFFER_SIZE];
		strncpy(mimeTypeCopy, mimeType, MIME_TYPE_NAME_BUFFER_SIZE);
		XdgMimeSubType *sub_type = _xdg_mime_sub_type_item_search(&(*group)->types, mimeTypeCopy);

		if (sub_type)
			return &sub_type->apps;
	}

	return 0;
}

const XdgArray *_xdg_mime_known_apps_lookup(XdgApplications *applications, const char *mimeType)
{
	char mimeTypeCopy[MIME_TYPE_NAME_BUFFER_SIZE];
	strncpy(mimeTypeCopy, mimeType, MIME_TYPE_NAME_BUFFER_SIZE);
	XdgMimeSubType *sub_type = _xdg_mime_sub_type_item_search(&applications->asoc_map, mimeTypeCopy);

	if (sub_type)
		return &sub_type->apps;
	else
		return 0;
}

const char *_xdg_mime_app_icon_lookup(XdgApplications *applications, const XdgApp *app, const char *themeName, int size)
{
	XdgTheme **hicolor = (XdgTheme **)search_node(&applications->themes_files_map, "hicolor");

	if (hicolor)
	{
		XdgTheme **theme = (XdgTheme **)search_node(&applications->themes_files_map, themeName);

		if (theme)
		{
			XdgAppGroup **group = (XdgAppGroup **)search_node(&app->groups, "Desktop Entry");

			if (group)
			{
				XdgAppGroupEntry **entry = (XdgAppGroupEntry **)search_node(&(*group)->entries, "Icon");

				if (entry && (*entry)->values.count)
					return _xdg_mime_find_icon((*entry)->values.list[0], size, *theme, *hicolor);
			}
		}
	}

	return 0;
}

const XdgAppGroup *xdg_mime_app_group_lookup(const XdgApp *app, const char *group)
{
	XdgAppGroup **res = (XdgAppGroup **)search_node(&app->groups, group);

	if (res)
		return (*res);
	else
		return 0;
}

const XdgArray *xdg_mime_app_entry_lookup(const XdgAppGroup *group, const char *entry)
{
	XdgAppGroupEntry **res = (XdgAppGroupEntry **)search_node(&group->entries, entry);

	if (res)
		return &(*res)->values;
	else
		return 0;
}

int xdg_mime_array_size(const XdgArray *array)
{
	return array->count;
}

const XdgApp *xdg_mime_array_app_item_at(const XdgArray *array, int index)
{
	return (XdgApp *)array->list[index];
}

const char *xdg_mime_array_string_item_at(const XdgArray *array, int index)
{
	return (char *)array->list[index];
}
