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

#include "xdgmimeapp.h"
#include "avltree.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>


#define GROUP_LIST_ALLOC_GRANULARITY             2
#define GROUP_ENTRIES_LIST_ALLOC_GRANULARITY     4
#define GROUP_ENTRY_VALUE_LIST_ALLOC_GRANULARITY 2
#define APP_ARRAY_ALLOC_GRANULARITY              2
#define READ_FROM_FILE_BUFFER_SIZE               1024


/**
 * Data structures
 *
 */
struct XdgEntryValueList
{
	char **list;
	int count;
	int capacity;
};
typedef struct XdgEntryValueList XdgEntryValueList;


struct XdgGroupEntry
{
	XdgEntryValueList values;
};
typedef struct XdgGroupEntry XdgGroupEntry;


struct XdgGroup
{
	AvlTree entries;
};
typedef struct XdgGroup XdgGroup;


struct XdgApp
{
	AvlTree groups;
};
typedef struct XdgApp XdgApp;


struct XdgAppArray
{
	XdgApp **list;
	int count;
	int capacity;
};
typedef struct XdgAppArray XdgAppArray;


struct XdgMimeSubType
{
	XdgAppArray apps;
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


struct XdgApplications
{
	AvlTree app_files_map;
	AvlTree lst_files_map;
	AvlTree asoc_map;
};


/**
 * Memory allocation functions
 *
 */
static char **_xdg_entry_value_list_new(XdgEntryValueList *list)
{
	char **res;

	if (list->capacity == 0)
		if (list->list == NULL)
		{
			list->list = malloc(GROUP_ENTRY_VALUE_LIST_ALLOC_GRANULARITY * sizeof(char *));
			list->capacity = GROUP_ENTRY_VALUE_LIST_ALLOC_GRANULARITY;
		}
		else
		{
			list->list = realloc(list->list, list->count * 2 * sizeof(char *));
			list->capacity = list->count;
		}

	res = &list->list[list->count];
	++list->count;
	--list->capacity;

	return res;
}

static void _xdg_entry_value_list_add(XdgEntryValueList *list, const char *name)
{
	(*_xdg_entry_value_list_new(list)) = strdup(name);
}

static void _xdg_entry_value_list_free(XdgEntryValueList *list)
{
	if (list->list)
	{
		int i = 0, size = list->count;

		for (; i < size; ++i)
			free(list->list[i]);

		free(list->list);
	}
}

static XdgGroupEntry *_xdg_group_entry_map_item_add(AvlTree *map, const char *name)
{
	XdgGroupEntry **res = (XdgGroupEntry **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgGroupEntry));
		memset(&(*res)->values, 0, sizeof(XdgEntryValueList));
	}

	return (*res);
}

static void _xdg_group_entry_map_item_free(XdgGroupEntry *item)
{
	_xdg_entry_value_list_free(&item->values);
	free(item);
}

static XdgGroup *_xdg_group_map_item_add(AvlTree *map, const char *name)
{
	XdgGroup **res = (XdgGroup **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgGroup));
		init_avl_tree(&(*res)->entries, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_group_map_item_free(XdgGroup *item)
{
	clear_avl_tree_and_values(&item->entries, (DestroyValue)_xdg_group_entry_map_item_free);
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
	clear_avl_tree_and_values(&item->groups, (DestroyValue)_xdg_group_map_item_free);
	free(item);
}

static void _xdg_app_array_add(XdgAppArray *array, XdgApp *app)
{
	if (array->capacity == 0)
		if (array->list == NULL)
		{
			array->list = malloc(APP_ARRAY_ALLOC_GRANULARITY * sizeof(XdgApp *));
			array->capacity = APP_ARRAY_ALLOC_GRANULARITY;
		}
		else
		{
			array->list = realloc(array->list, array->count * 2 * sizeof(XdgApp *));
			array->capacity = array->count;
		}

	array->list[array->count] = app;
	++array->count;
	--array->capacity;
}

static void _xdg_app_array_free(XdgAppArray *array)
{
	free(array->list);
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
	_xdg_app_array_free(&sub_type->apps);
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

static void _xdg_app_group_read_entry(XdgGroup *group, XdgApp *app, AvlTree *asoc_map, const char *line)
{
	char *sep;
	XdgMimeSubType *sub_type;

	if ((sep = strchr(line, '=')) != NULL)
	{
		*sep = 0;
		XdgGroupEntry *entry = _xdg_group_entry_map_item_add(&group->entries, line);
		char *start = (++sep);

		if (strcmp(line, "MimeType") == 0)
		{
			for (; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_entry_value_list_add(&entry->values, start);

					if (sub_type = _xdg_mime_sub_type_item_read(asoc_map, start))
						_xdg_app_array_add(&sub_type->apps, app);

					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_entry_value_list_add(&entry->values, start);

				if (sub_type = _xdg_mime_sub_type_item_read(asoc_map, start))
					_xdg_app_array_add(&sub_type->apps, app);
			}
		}
		else
		{
			for (; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_entry_value_list_add(&entry->values, start);
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_entry_value_list_add(&entry->values, start);
			}
		}
	}
}

static void _xdg_mime_group_read_entry(AvlTree *app_files_map, XdgMimeGroup *group, const char *line)
{
	char *sep;

	if ((sep = strchr(line, '=')) != NULL)
	{
		*sep = 0;
		XdgMimeSubType *sub_type = _xdg_mime_sub_type_item_read(&group->types, line);

		if (sub_type)
		{
			char *start = (++sep);

			for (; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_app_array_add(&sub_type->apps, _xdg_app_map_item_add(app_files_map, start));
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_app_array_add(&sub_type->apps, _xdg_app_map_item_add(app_files_map, start));
			}
		}
	}
}

static void _xdg_applications_read_desktop_file(char *buffer, XdgApplications *applications, FILE *file, const char *file_name)
{
	char *sep;
	XdgApp *app;
	XdgGroup *group = NULL;

	app = _xdg_app_map_item_add(&applications->app_files_map, file_name);

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n') //GenericName[zh_CN]
			if (buffer[0] == '[')
			{
				group = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
				{
					*sep = 0;
					group = _xdg_group_map_item_add(&app->groups, buffer + 1);
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

static void __xdg_applications_read_from_directory(char *buffer, XdgApplications *applications, const char *directory_name)
{
	DIR *dir;

	if (dir = opendir(directory_name))
	{
		FILE *file;
		char *file_name;
		struct dirent *entry;

		while ((entry = readdir(dir)) != NULL)
			if (entry->d_type == DT_DIR)
			{
				if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
				{
					file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
					strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

					__xdg_applications_read_from_directory(buffer, applications, file_name);

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
							_xdg_applications_read_desktop_file(buffer, applications, file, entry->d_name);
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

	__xdg_applications_read_from_directory(buffer, applications, directory_name);
}

const char *_xdg_mime_applications_lookup(XdgApplications *applications, const char *mime)
{
	return 0;
}

XdgApplications *_xdg_mime_applications_new(void)
{
	XdgApplications *list;

	list = malloc(sizeof(XdgApplications));
	init_avl_tree(&list->app_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&list->lst_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&list->asoc_map, strdup, (DestroyKey)free, strcmp);

	return list;
}

void _xdg_mime_applications_free(XdgApplications *applications)
{
	clear_avl_tree_and_values(&applications->app_files_map, (DestroyValue)_xdg_app_map_item_free);
	clear_avl_tree_and_values(&applications->lst_files_map, (DestroyValue)_xdg_mime_group_map_item_free);
	clear_avl_tree_and_values(&applications->asoc_map, (DestroyValue)_xdg_mime_type_map_item_free);
	free(applications);
}

void _xdg_mime_applications_dump(XdgApplications *applications)
{

}
