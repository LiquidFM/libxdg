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
struct XdgEntryValue
{
	char *value;
};
typedef struct XdgEntryValue XdgEntryValue;


struct XdgEntryValueList
{
	XdgEntryValue *list;
	int count;
	int capacity;
};
typedef struct XdgEntryValueList XdgEntryValueList;


struct XdgGroupEntry
{
	char *name;
	XdgEntryValueList values;
};
typedef struct XdgGroupEntry XdgGroupEntry;


struct XdgGroupEntryList
{
	XdgGroupEntry *list;
	int count;
	int capacity;
};
typedef struct XdgGroupEntryList XdgGroupEntryList;


struct XdgGroup
{
	char *name;
	XdgGroupEntryList entries;
};
typedef struct XdgGroup XdgGroup;


struct XdgGroupList
{
	XdgGroup *list;
	int count;
	int capacity;
};
typedef struct XdgGroupList XdgGroupList;


struct XdgApp
{
	XdgGroupList groups;
	char name[1];
};
typedef struct XdgApp XdgApp;


struct XdgAppArray
{
	char **list;
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
static XdgEntryValue *_xdg_mime_entry_value_list_add(XdgEntryValueList *list)
{
	XdgEntryValue *res;

	if (list->capacity == 0)
		if (list->list == NULL)
		{
			list->list = malloc(GROUP_ENTRY_VALUE_LIST_ALLOC_GRANULARITY * sizeof(XdgEntryValue));
			list->capacity = GROUP_ENTRY_VALUE_LIST_ALLOC_GRANULARITY;
		}
		else
		{
			list->list = realloc(list->list, list->count * 2 * sizeof(XdgEntryValue));
			list->capacity = list->count;
		}

	res = &list->list[list->count];
	++list->count;
	--list->capacity;

	memset(res, 0, sizeof(XdgEntryValue));
	return res;
}

static void _xdg_mime_entry_value_list_new_item(XdgEntryValueList *list, const char *name, size_t len)
{
	XdgEntryValue *value = _xdg_mime_entry_value_list_add(list);

	value->value = malloc(len + 1);
	memcpy(value->value, name, len);
	value->value[len] = 0;
}

static void _xdg_mime_entry_value_list_free(XdgEntryValueList *list)
{
	if (list->list)
	{
		int i = 0, size = list->count;

		for (; i < size; ++i)
			free(list->list[i].value);

		free(list->list);
	}
}

static XdgGroupEntry *_xdg_mime_group_entry_list_add(XdgGroupEntryList *list)
{
	XdgGroupEntry *res;

	if (list->capacity == 0)
		if (list->list == NULL)
		{
			list->list = malloc(GROUP_ENTRIES_LIST_ALLOC_GRANULARITY * sizeof(XdgGroupEntry));
			list->capacity = GROUP_ENTRIES_LIST_ALLOC_GRANULARITY;
		}
		else
		{
			list->list = realloc(list->list, list->count * 2 * sizeof(XdgGroupEntry));
			list->capacity = list->count;
		}

	res = &list->list[list->count];
	++list->count;
	--list->capacity;

	memset(res, 0, sizeof(XdgGroupEntry));
	return res;
}

static void _xdg_mime_group_entry_list_free(XdgGroupEntryList *list)
{
	if (list->list)
	{
		int i = 0, size = list->count;

		for (; i < size; ++i)
		{
			free(list->list[i].name);
			_xdg_mime_entry_value_list_free(&list->list[i].values);
		}

		free(list->list);
	}
}

static XdgGroupEntry *_xdg_mime_group_entry_list_new_item(XdgGroupEntryList *entries, const char *name, size_t len)
{
	XdgGroupEntry *entry = _xdg_mime_group_entry_list_add(entries);

	entry->name = malloc(len + 1);
	memcpy(entry->name, name, len);
	entry->name[len] = 0;

	return entry;
}

static XdgGroup *_xdg_mime_group_list_add(XdgGroupList *list)
{
	XdgGroup *res;

	if (list->capacity == 0)
		if (list->list == NULL)
		{
			list->list = malloc(GROUP_LIST_ALLOC_GRANULARITY * sizeof(XdgGroup));
			list->capacity = GROUP_LIST_ALLOC_GRANULARITY;
		}
		else
		{
			list->list = realloc(list->list, list->count * 2 * sizeof(XdgGroup));
			list->capacity = list->count;
		}

	res = &list->list[list->count];
	++list->count;
	--list->capacity;

	memset(res, 0, sizeof(XdgGroup));
	return res;
}

static void _xdg_mime_group_list_free(XdgGroupList *array)
{
	if (array->list)
	{
		int i = 0, size = array->count;

		for (; i < size; ++i)
		{
			free(array->list[i].name);
			_xdg_mime_group_entry_list_free(&array->list[i].entries);
		}

		free(array->list);
	}
}

static XdgGroup *_xdg_mime_group_list_new_item(XdgGroupList *groups, const char *name, size_t len)
{
	XdgGroup *group = _xdg_mime_group_list_add(groups);

	group->name = malloc(len + 1);
	memcpy(group->name, name, len);
	group->name[len] = 0;

	return group;
}

static XdgApp *_xdg_mime_app_map_item_new(AvlTree *app_files_map, const char *name)
{
	XdgApp *res = *(XdgApp **)search_or_create_node(app_files_map, name);

	if (res == NULL)
	{
		size_t len = strlen(name);

		res = malloc(sizeof(XdgApp) + len);
		memset(&res->groups, 0, sizeof(XdgGroupList));
		memcpy(res->name, name, len);
		res->name[len] = 0;
	}

	return res;
}

static void _xdg_mime_app_map_item_free(XdgApp *app)
{
	if (app->groups.list)
		_xdg_mime_group_list_free(&app->groups);

	free(app);
}

static void _xdg_mime_app_array_add(XdgAppArray *array, const char *name)
{
	if (array->capacity == 0)
		if (array->list == NULL)
		{
			array->list = malloc(APP_ARRAY_ALLOC_GRANULARITY * sizeof(const char *));
			array->capacity = APP_ARRAY_ALLOC_GRANULARITY;
		}
		else
		{
			array->list = realloc(array->list, array->count * 2 * sizeof(const char *));
			array->capacity = array->count;
		}

	array->list[array->count] = strdup(name);
	++array->count;
	--array->capacity;
}

static void _xdg_mime_app_array_free(XdgAppArray *array)
{
	if (array->list)
	{
		int i = 0, size = array->count;

		for (; i < size; ++i)
			free(array->list[i]);

		free(array->list);
	}
}

static XdgMimeSubType *_xdg_mime_sub_type_map_item_new(AvlTree *map, const char *name)
{
	XdgMimeSubType **res = (XdgMimeSubType **)search_or_create_node(map, name);

	if ((*res) == NULL)
		(*res) = calloc(1, sizeof(XdgMimeSubType));

	return (*res);
}

static void _xdg_mime_sub_type_map_item_free(XdgMimeSubType *sub_type)
{
	_xdg_mime_app_array_free(&sub_type->apps);
	free(sub_type);
}

static XdgMimeType *_xdg_mime_type_map_item_new(AvlTree *map, const char *name)
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

static XdgMimeGroup *_xdg_mime_group_map_item_new(AvlTree *map, const char *name)
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
static void _xdg_mime_app_read_group_entry(XdgGroup *group, const char *line)
{
	char *sep;

	if ((sep = strchr(line, '=')) != NULL)
	{
		XdgGroupEntry *entry = _xdg_mime_group_entry_list_new_item(&group->entries, line, sep - line);
		char *start = (++sep);

		for (; *sep && *sep != '\n'; ++sep)
			if (*sep == ';')
			{
				_xdg_mime_entry_value_list_new_item(&entry->values, start, sep - start);
				start = sep + 1;
			}

		if (*start != 0 && *start != '\n')
			_xdg_mime_entry_value_list_new_item(&entry->values, start, sep - start);
	}
}

static void _xdg_mime_group_read_entry(XdgMimeGroup *group, const char *line)
{
	char *sep;

	if ((sep = strchr(line, '/')) != NULL)
	{
		*sep = 0;
		XdgMimeType *type = _xdg_mime_type_map_item_new(&group->types, line);
		char *start = (++sep);

		if ((sep = strchr(sep, '=')) != NULL)
		{
			*sep = 0;
			XdgMimeSubType *sub_type = _xdg_mime_sub_type_map_item_new(&type->sub_types, start);
			start = (++sep);

			for (; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_mime_app_array_add(&sub_type->apps, start);
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_mime_app_array_add(&sub_type->apps, start);
			}
		}
	}
}

static void _xdg_mime_applications_read_desktop_file(char *buffer, XdgApplications *applications, FILE *file, const char *file_name)
{
	char *sep;
	XdgApp *app;
	XdgGroup *group = NULL;

	app = _xdg_mime_app_map_item_new(&applications->app_files_map, file_name);

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n')
			if (buffer[0] == '[')
			{
				group = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
					group = _xdg_mime_group_list_new_item(&app->groups, buffer + 1, sep - buffer - 1);
			}
			else
				if (group)
					_xdg_mime_app_read_group_entry(group, buffer);
				else
					break;
}

static void _xdg_mime_applications_read_list_file(char *buffer, XdgApplications *applications, FILE *file)
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
					group = _xdg_mime_group_map_item_new(&applications->lst_files_map, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_mime_group_read_entry(group, buffer);
				else
					break;
}

static void __xdg_mime_applications_read_from_directory(char *buffer, XdgApplications *applications, const char *directory_name)
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

					__xdg_mime_applications_read_from_directory(buffer, applications, file_name);

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
							_xdg_mime_applications_read_desktop_file(buffer, applications, file, entry->d_name);
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
								_xdg_mime_applications_read_list_file(buffer, applications, file);
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

	__xdg_mime_applications_read_from_directory(buffer, applications, directory_name);
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
	clear_avl_tree_and_values(&applications->app_files_map, (DestroyValue)_xdg_mime_app_map_item_free);
	clear_avl_tree_and_values(&applications->lst_files_map, (DestroyValue)_xdg_mime_group_map_item_free);
	clear_avl_tree_and_values(&applications->asoc_map, (DestroyValue)free);
	free(applications);
}

void _xdg_mime_applications_dump(XdgApplications *applications)
{

}
