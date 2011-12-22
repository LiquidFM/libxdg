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


struct XdgAppList
{
	XdgApp *list;
	int count;
	int capacity;
};
typedef struct XdgAppList XdgAppList;


struct XdgApplications
{
	AvlTree *app_map;
	AvlTree *assoc_map;
};


/**
 * Comparison functions
 *
 */
static int app_name_cmp(const void *v1, const void *v2)
{
	return strcmp(((XdgApp *)v1)->name, ((XdgApp *)v2)->name);
}


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

static void _xdg_mime_group_list_free(XdgGroupList *list)
{
	if (list->list)
	{
		int i = 0, size = list->count;

		for (; i < size; ++i)
		{
			free(list->list[i].name);
			_xdg_mime_group_entry_list_free(&list->list[i].entries);
		}

		free(list->list);
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

static XdgApp *_xdg_mime_app_map_item_new(AvlTree *apps, const char *name)
{
	void **test = search_or_create_node(apps, "1");
	test = search_or_create_node(apps, "2");
	test = search_or_create_node(apps, "3");
	test = search_or_create_node(apps, "4");
	test = search_or_create_node(apps, "5");
	test = search_or_create_node(apps, "6");
	test = search_or_create_node(apps, "7");
	test = search_or_create_node(apps, "8");
	test = search_or_create_node(apps, "9");
	test = search_or_create_node(apps, "10");




	XdgApp **app = (XdgApp **)search_or_create_node(apps, name);

	if ((*app) == NULL)
	{
		size_t len = strlen(name);

		(*app) = malloc(sizeof(XdgApp) + len);
		memset(&(*app)->groups, 0, sizeof(XdgGroupList));
		memcpy((*app)->name, name, len);
		(*app)->name[len] = 0;
	}

	return *app;
}

static void _xdg_mime_app_map_item_free(XdgApp *app)
{
	if (app->groups.list)
		_xdg_mime_group_list_free(&app->groups);

	free(app);
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

static void _xdg_mime_applications_read_desktop_file(char *buffer, XdgApplications *applications, FILE *file, const char *file_name)
{
	char *sep;
	XdgApp *app;
	XdgGroup *group = NULL;

	app = _xdg_mime_app_map_item_new(applications->app_map, file_name);

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
	XdgGroup *group = NULL;

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n')
			if (buffer[0] == '[')
			{
				group = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
				{
					*sep = 0;
//					group = search_or_create_node(applications->assoc_map, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_mime_app_read_group_entry(group, buffer);
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

	list = calloc(1, sizeof(XdgApplications));
	list->app_map = create_avl_tree(strdup, (DestroyKey)free, strcmp);
	list->assoc_map = create_avl_tree(strdup, (DestroyKey)free, strcmp);

	return list;
}

void _xdg_mime_applications_free(XdgApplications *applications)
{
	free_avl_tree_and_values(applications->app_map, (DestroyValue)_xdg_mime_app_map_item_free);
	free_avl_tree(applications->assoc_map);
}

void _xdg_mime_applications_dump(XdgApplications *applications)
{

}
