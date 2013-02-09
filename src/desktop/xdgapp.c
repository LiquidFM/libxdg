/** @internal @file xdgapp.c
 *  @brief Private file.
 *
 * Data structures for storing \a ".desktop" and \a ".list" files contents.
 *
 * More info can be found at http://www.freedesktop.org/standards/
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

#include "xdgapp_p.h"
#include "xdgappcache_p.h"
#include "xdgmimedefs.h"
#include "../basedirectory/xdgbasedirectory.h"

#ifdef THEMES_SPEC
#	include "../themes/xdgtheme.h"
#endif

#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <assert.h>


static const char cache_file_name[] = "applications.cache";


/**
 * Holds cache data
 */
struct XdgAppCache
{
	XdgAppCahceFile file;
	const XdgList *files;
	const AvlTree *asoc_map;
	const AvlTree *app_files_map;
	const AvlTree *lst_files_map;
};
typedef struct XdgAppCache XdgAppCache;


/**
 * Holds dynamically indexed data
 */
struct XdgAppData
{
	AvlTree asoc_map;
	AvlTree app_files_map;
	AvlTree lst_files_map;
	XdgList files;
};
typedef struct XdgAppData XdgAppData;


/**
 * Stores indexed data of \a ".desktop" and \a ".list" files
 * of a given folder.
 */
struct XdgAppFolder
{
	XdgAppData data;
	XdgAppCache cache;

	const AvlTree *asoc_map;
	const AvlTree *app_files_map;
	const AvlTree *lst_files_map;
};
typedef struct XdgAppFolder XdgAppFolder;


/**
 * Stores linked list of folders.
 */
struct XdgAppFolderItem
{
	XdgListItem item;
	XdgAppFolder app;
	char directory[1];
};
typedef struct XdgAppFolderItem XdgAppFolderItem;


/**
 * Just for passing arguments into _init_from_directory().
 */
struct InitFromDirectoryArgs
{
	char *buffer;
	XdgList *folders;
};
typedef struct InitFromDirectoryArgs InitFromDirectoryArgs;


/**
 * Just for passing arguments into _xdg_load_user_defined_apps().
 */
struct InitFromHomeDirectoryArgs
{
    char *buffer;
    XdgUserApps **apps;
};
typedef struct InitFromHomeDirectoryArgs InitFromHomeDirectoryArgs;


/**
 * Type of functions which is used for handling entry of
 * \a ".list" files.
 */
typedef void (*ReadListEntry)(const char *line, void *handler, void *user_data);


/**
 * Just for passing arguments into _xdg_app_read_list_file().
 */
struct ReadListFileArgs
{
    void *addedHandler;
    void *defaultHandler;
    void *removedHandler;
    void *userData;
    ReadListEntry handler;
};
typedef struct ReadListFileArgs ReadListFileArgs;


/**
 * Just for passing arguments into _xdg_user_defined_apps_save().
 */
struct UserDefinedAppsArgs
{
    const char *type;
    FILE *file;
};
typedef struct UserDefinedAppsArgs UserDefinedAppsArgs;


/**
 * Just for passing arguments into _rebuild_directory_cache().
 */
struct RebuildDirectoryCacheArgs
{
	char *buffer;
	RebuildCacheResult *result;
};
typedef struct RebuildDirectoryCacheArgs RebuildDirectoryCacheArgs;


/**
 * Type of functions which is used for handling the group of
 * \a ".list" files.
 */
typedef void (*GroupHandler)(XdgListItem **item, const char *name, XdgApp *app);


static XdgList folders_list = { NULL, NULL };


/**
 * Memory allocation functions
 */
static void _xdg_list_value_item_add(XdgList *list, const char *line)
{
	XdgValue *value = malloc(sizeof(XdgValue) + strlen(line));

	_xdg_list_apped(list, (XdgListItem *)value);
	strcpy(value->value, line);
}

static void _xdg_list_value_item_free(XdgList *list)
{
	_xdg_list_free(list, free);
}

static void _xdg_list_value_item_and_list_free(XdgList *list)
{
    _xdg_list_value_item_free(list);
    free(list);
}

static void _xdg_file_watcher_list_add(XdgList *list, const char *path, struct stat *st)
{
	XdgFileWatcher *res = malloc(sizeof(XdgFileWatcher) + strlen(path));

	_xdg_list_apped(list, (XdgListItem *)res);

	if (stat(path, st) == 0)
		res->mtime = st->st_mtime;
	else
		res->mtime = 0;

	strcpy(res->path, path);
}

static void _xdg_file_watcher_list_free(XdgList *list)
{
	_xdg_list_free(list, free);
}

void _xdg_list_app_item_append(XdgList *list, const char *name, XdgApp *app)
{
	XdgMimeSubTypeValue *value = malloc(sizeof(XdgMimeSubTypeValue) + strlen(name));

	_xdg_list_apped(list, (XdgListItem *)value);

	value->app = app;
	strcpy(value->name, name);
}

static XdgMimeSubTypeValue *_xdg_list_app_item_append_copy(XdgList *list, XdgMimeSubTypeValue *value)
{
    XdgMimeSubTypeValue *res = malloc(sizeof(XdgMimeSubTypeValue) + strlen(value->name));

    _xdg_list_apped(list, (XdgListItem *)res);

    res->app = value->app;
    strcpy(res->name, value->name);

    return res;
}

void _xdg_list_app_item_prepend(XdgList *list, const char *name, XdgApp *app)
{
    XdgMimeSubTypeValue *value = malloc(sizeof(XdgMimeSubTypeValue) + strlen(name));

    _xdg_list_prepend(list, (XdgListItem *)value);

    value->app = app;
    strcpy(value->name, name);
}

static XdgMimeSubTypeValue *_xdg_list_app_item_prepend_copy(XdgList *list, XdgMimeSubTypeValue *value)
{
    XdgMimeSubTypeValue *res = malloc(sizeof(XdgMimeSubTypeValue) + strlen(value->name));

    _xdg_list_prepend(list, (XdgListItem *)res);

    res->app = value->app;
    strcpy(res->name, value->name);

    return res;
}

static void _xdg_mime_sub_type_map_item_free(XdgMimeSubType *sub_type)
{
	_xdg_list_free((XdgList *)&sub_type->apps, free);
	free(sub_type);
}

static void _xdg_mime_type_map_item_free(XdgMimeType *type)
{
	clear_avl_tree_and_values(&type->sub_types, (DestroyValue)_xdg_mime_sub_type_map_item_free);
	free(type);
}

static XdgAppGroupEntry *_xdg_app_group_entry_map_item_add(AvlTree *map, const char *name)
{
	XdgAppGroupEntry **res = (XdgAppGroupEntry **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgAppGroupEntry));
		memset(&(*res)->values, 0, sizeof(XdgListItem));
		init_avl_tree(&(*res)->localized, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_app_group_entry_map_item_free(XdgAppGroupEntry *item)
{
	_xdg_list_value_item_free(&item->values);
	clear_avl_tree_and_values(&item->localized, (DestroyValue)_xdg_list_value_item_and_list_free);
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

static XdgApp *_xdg_app_map_item_find(AvlTree *map, const char *name)
{
	XdgApp **res = (XdgApp **)search_or_create_node(map, name);

	if ((*res) == NULL)
		(*res) = calloc(1, sizeof(XdgApp));

	return (*res);
}

static XdgApp *_xdg_app_map_item_find_const(const AvlTree *map, const char *name)
{
    XdgApp **res = (XdgApp **)search_node(map, name);

    if (res == NULL)
        return NULL;
    else
        return (*res);
}

static XdgApp *_xdg_app_map_item_add(AvlTree *map, const char *name)
{
	XdgApp *res = _xdg_app_map_item_find(map, name);

	if (res->groups == NULL)
	{
		res->groups = malloc(sizeof(XdgAppGroups));
		res->groups->owner = res;
		init_avl_tree(&res->groups->tree, strdup, (DestroyKey)free, strcmp);
	}

	return res;
}

static void _xdg_app_map_item_free(XdgApp *item)
{
	if (item->groups && item->groups->owner == item)
	{
		clear_avl_tree_and_values(&item->groups->tree, (DestroyValue)_xdg_app_group_map_item_free);
		free(item->groups);
	}

	free(item);
}

static void _xdg_app_cache_init(XdgAppCache *cache)
{
	memset(cache, 0, sizeof(XdgAppCache));
}

static int _xdg_app_cache_read(XdgAppCache *cache)
{
	void *memory = cache->file.memory;

	if (read_version(&memory) == 1)
	{
		cache->files = read_file_watcher_list(&memory);
		cache->app_files_map = map_from_memory(&memory, (ReadKey)read_app_key, (ReadValue)read_app, strcmp, NULL);
		cache->asoc_map = map_from_memory(&memory, (ReadKey)read_app_key, (ReadValue)read_mime_group_type, strcmp, (void *)cache->app_files_map);
		cache->lst_files_map = map_from_memory(&memory, (ReadKey)read_app_key, (ReadValue)read_mime_group_type, strcmp, (void *)cache->app_files_map);

		return TRUE;
	}

	return FALSE;
}

static void _xdg_app_cache_write(XdgAppCahceFile *file, XdgAppData *data)
{
	write_version(file->fd, 1);
	write_file_watcher_list(file->fd, &data->files);
	write_to_file(file->fd, &data->app_files_map, write_app_key, (WriteValue)write_app, NULL);
	write_to_file(file->fd, &data->asoc_map, write_app_key, (WriteValue)write_mime_group_type, NULL);
	write_to_file(file->fd, &data->lst_files_map, write_app_key, (WriteValue)write_mime_group_type, NULL);
}

static void _xdg_app_cache_free(XdgAppCache *cache)
{
	_xdg_app_cache_close(&cache->file);
	memset(cache, 0, sizeof(XdgAppCache));
}

static void _xdg_app_data_init(XdgAppData *data)
{
	init_avl_tree(&data->asoc_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&data->app_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&data->lst_files_map, strdup, (DestroyKey)free, strcmp);
	data->files.head = data->files.tail = NULL;
}

static void _xdg_app_data_free(XdgAppData *data)
{
	clear_avl_tree_and_values(&data->asoc_map, (DestroyValue)_xdg_mime_type_map_item_free);
	clear_avl_tree_and_values(&data->app_files_map, (DestroyValue)_xdg_app_map_item_free);
	clear_avl_tree_and_values(&data->lst_files_map, (DestroyValue)_xdg_mime_type_map_item_free);
	_xdg_file_watcher_list_free(&data->files);
}

static XdgAppFolderItem *_xdg_app_folder_item_new(XdgList *list, const char *directory)
{
    XdgAppFolderItem *res = malloc(sizeof(XdgAppFolderItem) + strlen(directory));

	_xdg_list_apped(list, (XdgListItem *)res);
	_xdg_app_data_init(&res->app.data);
	_xdg_app_cache_init(&res->app.cache);
	strcpy(res->directory, directory);

	return res;
}

static void _xdg_app_folder_item_free(XdgAppFolderItem *folder)
{
	_xdg_app_data_free(&folder->app.data);
	_xdg_app_cache_free(&folder->app.cache);
	free(folder);
}


/**
 * Main algorithms
 *
 */
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

static XdgMimeSubType *_xdg_mime_sub_type_map_item_add(AvlTree *map, const char *name)
{
	XdgMimeSubType **res = (XdgMimeSubType **)search_or_create_node(map, name);

	if ((*res) == NULL)
		(*res) = calloc(1, sizeof(XdgMimeSubType));

	return (*res);
}

XdgMimeSubType *_xdg_mime_sub_type_add(AvlTree *map, const char *name)
{
	char *sep;

	if ((sep = strchr(name, '/')) != NULL)
	{
		*sep = 0;
		XdgMimeType *type = _xdg_mime_type_map_item_add(map, name);
		char *start = sep + 1;
		*sep = '/';

		if (*start && *start != '\n')
			return _xdg_mime_sub_type_map_item_add(&type->sub_types, start);
	}

	return NULL;
}

static XdgMimeSubType *_xdg_mime_sub_type_item_search(const AvlTree *map, const char *type, const char *sub_type)
{
	XdgMimeType **mime_type = (XdgMimeType **)search_node(map, type);

	if (mime_type)
	{
		XdgMimeSubType **res = (XdgMimeSubType **)search_node(&(*mime_type)->sub_types, sub_type);

		if (res)
			return (*res);
	}

	return NULL;
}

static XdgMimeSubType *_xdg_mime_sub_type_item_search_or_create(AvlTree *map, const char *type, const char *sub_type)
{
    XdgMimeType *mime_type = _xdg_mime_type_map_item_add(map, type);
    return _xdg_mime_sub_type_map_item_add(&mime_type->sub_types, sub_type);
}

static void _xdg_app_group_read_entry_value(XdgList *list, char *line)
{
	char *sep;

	for (sep = line; *sep && *sep != '\n'; ++sep)
		if (*sep == ';')
		{
			*sep = 0;
			_xdg_list_value_item_add(list, line);
			line = sep + 1;
		}

	if (*line != 0 && *line != '\n')
	{
		*sep = 0;
		_xdg_list_value_item_add(list, line);
	}
}

static void __xdg_app_group_read_entry_value(XdgList **list, char *line)
{
    if ((*list) == NULL)
        (*list) = calloc(1, sizeof(XdgList));

    _xdg_app_group_read_entry_value(*list, line);
}

static void _xdg_app_group_read_mime_type_entry_value(XdgList *list, XdgApp *app, const char *app_name, AvlTree *asoc_map, char *line)
{
	char *sep;
	XdgMimeSubType *sub_type;

	for (sep = line; *sep && *sep != '\n'; ++sep)
		if (*sep == ';')
		{
			*sep = 0;
			_xdg_list_value_item_add(list, line);

			if (sub_type = _xdg_mime_sub_type_add(asoc_map, line))
				_xdg_list_app_item_append((XdgList *)&sub_type->apps, app_name, app);

			line = sep + 1;
		}

	if (*line != 0 && *line != '\n')
	{
		*sep = 0;
		_xdg_list_value_item_add(list, line);

		if (sub_type = _xdg_mime_sub_type_add(asoc_map, line))
			_xdg_list_app_item_append((XdgList *)&sub_type->apps, app_name, app);
	}
}

static void _xdg_app_group_read_entry(XdgAppGroup *group, XdgApp *app, const char *app_name, AvlTree *asoc_map, const char *line)
{
	char *sep;

	if ((sep = strchr(line, '=')) != NULL)
	{
		XdgListItem *values;
		char *start = sep + 1;
		REMOVE_WHITE_SPACES_LEFT(line, sep)
		REMOVE_WHITE_SPACES_RIGHT(start)

		if ((sep = strchr(line, '[')) != NULL)
		{
			*sep = 0;
			char *locale = sep + 1;
			XdgAppGroupEntry *entry = _xdg_app_group_entry_map_item_add(&group->entries, line);

			if ((sep = strchr(locale, ']')) != NULL)
			{
				*sep = 0;
				char *modifier = NULL;
				char buffer[LOCALE_NAME_BUFFER_SIZE];

				if ((sep = strchr(locale, '@')) != NULL)
				{
					*sep = 0;
					modifier = sep + 1;
				}

				strcpy(buffer, locale);

				if (modifier)
				{
					strcat(buffer, "@");
					strcat(buffer, modifier);
				}

				__xdg_app_group_read_entry_value((XdgList **)search_or_create_node(&entry->localized, buffer), start);
			}
			else
				_xdg_app_group_read_entry_value((XdgList *)&entry->values, start);
		}
		else
		{
			XdgAppGroupEntry *entry = _xdg_app_group_entry_map_item_add(&group->entries, line);

			if (strcmp(line, "MimeType") == 0)
				_xdg_app_group_read_mime_type_entry_value((XdgList *)&entry->values, app, app_name, asoc_map, start);
			else
				_xdg_app_group_read_entry_value((XdgList *)&entry->values, start);
		}
	}
}

static void _xdg_lst_group_handle_default(XdgList *list, const char *name, XdgApp *app)
{
    XdgMimeSubTypeValue *item = (XdgMimeSubTypeValue *)list->head;

    while (item)
        if (strcmp(item->name, name) == 0)
            return;
        else
            item = (XdgMimeSubTypeValue *)item->item.item.next;

	_xdg_list_app_item_append(list, name, app);
}

static int _xdg_lst_remove_sub_type_value(XdgMimeSubTypeValue *item, const char *name)
{
	return strcmp(item->name, name) == 0;
}

static void _xdg_lst_group_handle_added(XdgList *list, const char *name, XdgApp *app)
{
	_xdg_list_remove_if(list, (XdgListItemMatch)_xdg_lst_remove_sub_type_value, (void *)name, free);
	_xdg_list_app_item_prepend(list, name, app);
}

static void _xdg_lst_group_handle_removed(XdgList *list, const char *name, XdgApp *app)
{
	_xdg_list_remove_if(list, (XdgListItemMatch)_xdg_lst_remove_sub_type_value, (void *)name, free);
}

static void _xdg_lst_group_read_entry(const char *line, GroupHandler handler, XdgAppData *data)
{
	char *sep;

	if ((sep = strchr(line, '=')) != NULL)
	{
		char *start = sep + 1;
		REMOVE_WHITE_SPACES_LEFT(line, sep)
		REMOVE_WHITE_SPACES_RIGHT(start)
		XdgMimeSubType *sub_type = _xdg_mime_sub_type_add(&data->lst_files_map, line);

		if (sub_type)
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					handler((XdgListItem **)&sub_type->apps, start, _xdg_app_map_item_find(&data->app_files_map, start));
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				handler((XdgListItem **)&sub_type->apps, start, _xdg_app_map_item_find(&data->app_files_map, start));
			}
		}
	}
}

static XdgApp *_xdg_folders_list_find_app(const char *name)
{
    XdgApp *res = NULL;
    XdgAppFolderItem *item = (XdgAppFolderItem *)folders_list.head;

    do
    {
        if (res = _xdg_app_map_item_find_const(item->app.app_files_map, name))
            break;

        item = (XdgAppFolderItem *)item->item.next;
    }
    while (item);

    return res;
}

static void _xdg_lst_group_read_entry_const(const char *line, AvlTree *lst_files_map)
{
    XdgApp *app;
    char *sep;

    if ((sep = strchr(line, '=')) != NULL)
    {
        char *start = sep + 1;
        REMOVE_WHITE_SPACES_LEFT(line, sep)
        REMOVE_WHITE_SPACES_RIGHT(start)
        XdgMimeSubType *sub_type = _xdg_mime_sub_type_add(lst_files_map, line);

        if (sub_type)
        {
            for (sep = start; *sep && *sep != '\n'; ++sep)
                if (*sep == ';')
                {
                    *sep = 0;

                    if (app = _xdg_folders_list_find_app(start))
                        _xdg_lst_group_handle_default((XdgList *)&sub_type->apps, start, app);

                    start = sep + 1;
                }

            if (*start != 0 && *start != '\n')
            {
                *sep = 0;

                if (app = _xdg_folders_list_find_app(start))
                    _xdg_lst_group_handle_default((XdgList *)&sub_type->apps, start, app);
            }
        }
    }
}

static void _xdg_app_read_desktop_file(char *buffer, XdgAppData *data, FILE *file, const char *name)
{
	char *sep;
	XdgApp *app;
	XdgAppGroup *group = NULL;

	app = _xdg_app_map_item_add(&data->app_files_map, name);

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n')
			if (buffer[0] == '[')
			{
				group = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
				{
					*sep = 0;
					group = _xdg_app_group_map_item_add(&app->groups->tree, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_app_group_read_entry(group, app, name, &data->asoc_map, buffer);
				else
					break;
}

static void _xdg_app_read_list_file(char *buffer, FILE *file, ReadListFileArgs *args)
{
	char *sep;
	void *handler = NULL;

	while (fgets(buffer, READ_FROM_FILE_BUFFER_SIZE, file) != NULL)
		if (buffer[0] != '#' && buffer[0] != '\r' && buffer[0] != '\n')
			if (buffer[0] == '[')
			{
				handler = NULL;

				if ((sep = strchr(buffer, ']')) != NULL)
				{
					*sep = 0;

					if (strcmp(buffer + 1, "Default Applications") == 0)
						handler = args->defaultHandler;
					else
						if (strcmp(buffer + 1, "Added Associations") == 0)
							handler = args->addedHandler;
						else
							if (strcmp(buffer + 1, "Removed Associations") == 0)
								handler = args->removedHandler;
				}
			}
			else
				if (handler)
				    args->handler(buffer, handler, args->userData);
				else
					break;
}

static void __xdg_app_read_from_directory(char *buffer, XdgAppData *data, const char *directory_name, const char *preffix)
{
	DIR *dir;
	struct stat st;

	_xdg_file_watcher_list_add(&data->files, directory_name, &st);

	if (dir = opendir(directory_name))
	{
		FILE *file;
		char *file_name;
		struct dirent *entry;
		char *file_name_preffix;
		ReadListFileArgs args =
		{
		        _xdg_lst_group_handle_added,
		        _xdg_lst_group_handle_default,
		        _xdg_lst_group_handle_removed,
		        data,
		        (ReadListEntry)_xdg_lst_group_read_entry
		};

		while ((entry = readdir(dir)) != NULL)
			if (entry->d_type == DT_DIR)
			{
				if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
				{
					file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
					strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

					file_name_preffix = malloc(strlen(preffix) + strlen(entry->d_name) + 2);
					strcpy(file_name_preffix, preffix); strcat(file_name_preffix, entry->d_name); strcat(file_name_preffix, "-");

					__xdg_app_read_from_directory(buffer, data, file_name, file_name_preffix);

					free(file_name_preffix);
					free(file_name);
				}
			}
			else
				if (entry->d_type == DT_REG)
				{
					if (fnmatch("*.desktop", entry->d_name, FNM_NOESCAPE) != FNM_NOMATCH)
					{
						file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
						strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

						if (file = fopen(file_name, "r"))
						{
							_xdg_file_watcher_list_add(&data->files, file_name, &st);

							file_name_preffix = malloc(strlen(preffix) + strlen(entry->d_name) + 1);
							strcpy(file_name_preffix, preffix); strcat(file_name_preffix, entry->d_name);

							_xdg_app_read_desktop_file(buffer, data, file, file_name_preffix);

							free(file_name_preffix);
							fclose(file);
						}

						free(file_name);
					}
					else
						if (fnmatch("*.item", entry->d_name, FNM_NOESCAPE) != FNM_NOMATCH)
						{
							file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
							strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

							if (file = fopen(file_name, "r"))
							{
								_xdg_file_watcher_list_add(&data->files, file_name, &st);
								_xdg_app_read_list_file(buffer, file, &args);
								fclose(file);
							}

							free(file_name);
						}
				}
				else
                    if (entry->d_type == 0 && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                    {
                        file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
                        strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

                        if (lstat(file_name, &st) == 0)
                            if (S_ISDIR(st.st_mode))
                            {
                                file_name_preffix = malloc(strlen(preffix) + strlen(entry->d_name) + 2);
                                strcpy(file_name_preffix, preffix); strcat(file_name_preffix, entry->d_name); strcat(file_name_preffix, "-");

                                __xdg_app_read_from_directory(buffer, data, file_name, file_name_preffix);

                                free(file_name_preffix);
                            }
                            else
                                if (S_ISREG(st.st_mode) && fnmatch("*.desktop", entry->d_name, FNM_NOESCAPE) != FNM_NOMATCH)
                                {
                                    if (file = fopen(file_name, "r"))
                                    {
                                        _xdg_file_watcher_list_add(&data->files, file_name, &st);

                                        file_name_preffix = malloc(strlen(preffix) + strlen(entry->d_name) + 1);
                                        strcpy(file_name_preffix, preffix); strcat(file_name_preffix, entry->d_name);

                                        _xdg_app_read_desktop_file(buffer, data, file, file_name_preffix);

                                        free(file_name_preffix);
                                        fclose(file);
                                    }
                                }

                        free(file_name);
                    }

		closedir(dir);
	}
}

static void _xdg_app_read_from_directory(InitFromDirectoryArgs *args, const char *directory_name, const char *preffix)
{
	char *file_name;
	XdgAppFolderItem *folder = _xdg_app_folder_item_new(args->folders, directory_name);

	file_name = malloc(strlen(directory_name) + strlen(cache_file_name) + 2);
	strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, cache_file_name);

	_xdg_app_cache_new(&folder->app.cache.file, file_name);

	free(file_name);

	if (folder->app.cache.file.error == 0)
		if (_xdg_app_cache_read(&folder->app.cache))
		{
			folder->app.asoc_map = folder->app.cache.asoc_map;
			folder->app.app_files_map = folder->app.cache.app_files_map;
			folder->app.lst_files_map = folder->app.cache.lst_files_map;

			return;
		}
		else
			_xdg_app_cache_close(&folder->app.cache.file);

	__xdg_app_read_from_directory(args->buffer, &folder->app.data, directory_name, preffix);

	folder->app.asoc_map = &folder->app.data.asoc_map;
	folder->app.app_files_map = &folder->app.data.app_files_map;
	folder->app.lst_files_map = &folder->app.data.lst_files_map;
}

static int _init_from_directory(const char *directory, InitFromDirectoryArgs *user_data)
{
	char *file_name;
	assert(directory != NULL);

	file_name = malloc (strlen (directory) + strlen ("/applications") + 1);
	strcpy (file_name, directory); strcat (file_name, "/applications");

	_xdg_app_read_from_directory(user_data, file_name, "");

	free (file_name);

	return FALSE; /* Keep processing */
}

static int _rebuild_directory_cache(const char *directory, RebuildDirectoryCacheArgs *user_data)
{
	struct stat st;
	char *file_name;
	assert(directory != NULL);

	file_name = malloc (strlen (directory) + strlen ("/applications") + 1);
	strcpy (file_name, directory); strcat (file_name, "/applications");

	if (stat(file_name, &st) == 0 || errno != ENOENT)
		if ((user_data->result->error = xdg_app_rebuild_cache_file(file_name)) != 0)
		{
			user_data->result->directory = file_name;
			return TRUE; /* Stop processing */
		}

	free (file_name);
	return FALSE; /* Keep processing */
}

static void _xdg_app_fixup_links_between_folders(const char *name, XdgApp *app, const AvlTree *app_files_map)
{
	XdgApp **res = (XdgApp **)search_node(app_files_map, name);

	if (res)
		if (app->groups == NULL)
			app->groups = (*res)->groups;
		else
			(*res)->groups = app->groups;
}

static void _xdg_app_fixup_links_to_desktop_files()
{
	assert(!_is_empty_list(&folders_list) && "Library was not initialized!");

	XdgAppFolderItem *next;
	XdgAppFolderItem *folder = (XdgAppFolderItem *)folders_list.head;
	const AvlTree *app_files_map;

	do
	{
		next = (XdgAppFolderItem *)folder->item.next;

		while (next)
		{
			if (!_is_empty_list(&next->app.data.files))
				app_files_map = &next->app.data.app_files_map;
			else
				app_files_map = next->app.cache.app_files_map;

			if (is_empty_tree(app_files_map) == FALSE)
				if (!_is_empty_list(&folder->app.data.files))
					depth_first_search(&folder->app.data.app_files_map, (DepthFirstSearch)_xdg_app_fixup_links_between_folders, (void *)app_files_map);
				else
					depth_first_search(folder->app.cache.app_files_map, (DepthFirstSearch)_xdg_app_fixup_links_between_folders, (void *)app_files_map);

			next = (XdgAppFolderItem *)next->item.next;
		}

		folder = (XdgAppFolderItem *)folder->item.next;
	}
	while (folder);
}

static void _xdg_app_cleanup_links_between_folders(const char *name, XdgApp *app)
{
	if (app->groups && app->groups->owner != app)
		app->groups = NULL;
}

static void _xdg_app_cleanup_links_to_desktop_files()
{
	assert(!_is_empty_list(&folders_list) && "Library was not initialized!");
	XdgAppFolderItem *folder = (XdgAppFolderItem *)folders_list.head;

	do
	{
		if (!_is_empty_list(&folder->app.data.files))
			depth_first_search(&folder->app.data.app_files_map, (DepthFirstSearch)_xdg_app_cleanup_links_between_folders, NULL);
		else
			depth_first_search(folder->app.cache.app_files_map, (DepthFirstSearch)_xdg_app_cleanup_links_between_folders, NULL);

		folder = (XdgAppFolderItem *)folder->item.next;
	}
	while (folder);
}

void _xdg_app_init()
{
	char buffer[READ_FROM_FILE_BUFFER_SIZE];
	InitFromDirectoryArgs args = {buffer, &folders_list};

	_xdg_for_each_data_dir((XdgDirectoryFunc)_init_from_directory, &args);

	_xdg_app_fixup_links_to_desktop_files();
}

void _xdg_app_shutdown()
{
	_xdg_list_free(&folders_list, (XdgListItemFree)_xdg_app_folder_item_free);
}

static BOOL _xdg_check_time_stamp(const XdgList *files)
{
	struct stat st;
	XdgFileWatcher *file = (XdgFileWatcher *)files->head;

	while (file)
	{
		if (stat(file->path, &st) == 0)
		{
			if (st.st_mtime != file->mtime)
				return FALSE;
		}
		else
			if (file->mtime)
				return FALSE;

		file = (XdgFileWatcher *)file->item.next;
	}

	return TRUE;
}

int xdg_app_cache_file_is_valid(const char *directory)
{
	int res = FALSE;
	char *file_name;
	XdgAppCache cache;

	_xdg_app_cache_init(&cache);

	file_name = malloc(strlen(directory) + strlen(cache_file_name) + 2);
	strcpy(file_name, directory); strcat(file_name, "/"); strcat(file_name, cache_file_name);

	_xdg_app_cache_new(&cache.file, file_name);

	free(file_name);

	if (cache.file.error == 0)
	{
		void *memory = cache.file.memory;

		if (read_version(&memory) == 1)
		{
			cache.files = read_file_watcher_list(&memory);
			res = _xdg_check_time_stamp(cache.files);
		}

		_xdg_app_cache_close(&cache.file);
	}

	return res;
}

int xdg_app_rebuild_cache_file(const char *directory)
{
	int res = 0;
	char *file_name;
	XdgAppCache cache;

	_xdg_app_cache_init(&cache);

	file_name = malloc(strlen(directory) + strlen(cache_file_name) + 2);
	strcpy(file_name, directory); strcat(file_name, "/"); strcat(file_name, cache_file_name);

	_xdg_app_cache_new_empty(&cache.file, file_name);

	free(file_name);

	if (cache.file.error == 0)
	{
		XdgAppData data;
		char buffer[READ_FROM_FILE_BUFFER_SIZE];

		_xdg_app_data_init(&data);

		__xdg_app_read_from_directory(buffer, &data, directory, "");

		_xdg_app_cache_write(&cache.file, &data);

		_xdg_app_data_free(&data);
	}
	else
		res = cache.file.error;

	_xdg_app_cache_free(&cache);

	return res;
}

void xdg_app_rebuild_cache_in_each_data_dir(RebuildCacheResult *result)
{
	assert(result && "Argument \"result\" is NULL!");
	char buffer[READ_FROM_FILE_BUFFER_SIZE];
	RebuildDirectoryCacheArgs args = {buffer, result};

	_xdg_for_each_data_dir((XdgDirectoryFunc)_rebuild_directory_cache, &args);
}

static BOOL _xdg_load_cache(XdgAppFolderItem *folder)
{
	char *file_name;
	XdgAppCache cache;

	_xdg_app_cache_init(&cache);

	file_name = malloc(strlen(folder->directory) + strlen(cache_file_name) + 2);
	strcpy(file_name, folder->directory); strcat(file_name, "/"); strcat(file_name, cache_file_name);

	_xdg_app_cache_new(&cache.file, file_name);

	free(file_name);

	if (cache.file.error == 0)
		if (_xdg_app_cache_read(&cache))
		{
			_xdg_app_cache_free(&folder->app.cache);
			memcpy(&folder->app.cache, &cache, sizeof(XdgAppCache));

			folder->app.asoc_map = folder->app.cache.asoc_map;
			folder->app.app_files_map = folder->app.cache.app_files_map;
			folder->app.lst_files_map = folder->app.cache.lst_files_map;

			return TRUE;
		}
		else
			_xdg_app_cache_close(&cache.file);

	return FALSE;
}

void xdg_app_refresh(RebuildResult *result)
{
	assert(!_is_empty_list(&folders_list) && "Library was not initialized!");
	char buffer[READ_FROM_FILE_BUFFER_SIZE];
	XdgAppFolderItem *folder = (XdgAppFolderItem *)folders_list.head;

	_xdg_app_cleanup_links_to_desktop_files();

	do
	{
		if (!_is_empty_list(&folder->app.data.files))
		{
			if (_xdg_check_time_stamp(&folder->app.data.files) == FALSE)
				if (_xdg_load_cache(folder) == FALSE)
				{
					_xdg_app_data_free(&folder->app.data);
					_xdg_app_data_init(&folder->app.data);

					__xdg_app_read_from_directory(buffer, &folder->app.data, folder->directory, "");
				}
		}
		else
			if (_xdg_check_time_stamp(folder->app.cache.files) == FALSE)
				if (_xdg_load_cache(folder) == FALSE)
				{
					_xdg_app_data_free(&folder->app.data);
					_xdg_app_data_init(&folder->app.data);

					__xdg_app_read_from_directory(buffer, &folder->app.data, folder->directory, "");
				}

		folder = (XdgAppFolderItem *)folder->item.next;
	}
	while (folder);

	_xdg_app_fixup_links_to_desktop_files();
}

const XdgJointListItem *xdg_apps_lookup(const char *mimeType)
{
	assert(!_is_empty_list(&folders_list) && "Library was not initialized!");
	char buffer[MIME_TYPE_NAME_BUFFER_SIZE];
	XdgJointList res = { { NULL, NULL }, NULL, NULL };
	char *sep;

	if ((sep = strchr(mimeType, '/')) != NULL)
	{
		strncpy(buffer, mimeType, sep - mimeType);
		buffer[sep - mimeType] = 0;
		++sep;

		XdgMimeSubType *sub_type;
		XdgAppFolderItem *item = (XdgAppFolderItem *)folders_list.head;

		do
		{
			sub_type = _xdg_mime_sub_type_item_search(item->app.lst_files_map, buffer, sep);

			if (sub_type)
				_xdg_joint_list_apped(&res, (XdgJointListItem *)&sub_type->apps.list.tail);

			item = (XdgAppFolderItem *)item->item.next;
		}
		while (item);
	}

    if (res.head)
        return (const XdgJointListItem *)res.head->item.list->head;
    else
        return NULL;
}

const XdgJointListItem *xdg_known_apps_lookup(const char *mimeType)
{
	assert(!_is_empty_list(&folders_list) && "Library was not initialized!");
	char buffer[MIME_TYPE_NAME_BUFFER_SIZE];
	XdgJointList res = { { NULL, NULL }, NULL, NULL };
	char *sep;

	if ((sep = strchr(mimeType, '/')) != NULL)
	{
		strncpy(buffer, mimeType, sep - mimeType);
		buffer[sep - mimeType] = 0;
		++sep;

		XdgMimeSubType *sub_type;
		XdgAppFolderItem *item = (XdgAppFolderItem *)folders_list.head;

		do
		{
			sub_type = _xdg_mime_sub_type_item_search(item->app.asoc_map, buffer, sep);

			if (sub_type)
				_xdg_joint_list_apped(&res, (XdgJointListItem *)sub_type->apps.list.tail);

			item = (XdgAppFolderItem *)item->item.next;
		}
		while (item);
	}

	if (res.head)
	    return (const XdgJointListItem *)res.head->item.list->head;
	else
	    return NULL;
}

static void _xdg_load_user_defined_apps(const char *directory, InitFromHomeDirectoryArgs *args)
{
    FILE *file;
    char *file_name;
    assert(directory != NULL);

    *args->apps = malloc(sizeof(XdgUserApps) + strlen(directory) + strlen("/applications/mimeapps.list"));
    strcpy((*args->apps)->fileName, directory); strcat((*args->apps)->fileName, "/applications/mimeapps.list");

    init_avl_tree(&(*args->apps)->addedApps, strdup, (DestroyKey)free, strcmp);
    init_avl_tree(&(*args->apps)->defaultApps, strdup, (DestroyKey)free, strcmp);
    init_avl_tree(&(*args->apps)->removedApps, strdup, (DestroyKey)free, strcmp);

    if (file = fopen((*args->apps)->fileName, "r"))
    {
        ReadListFileArgs args2 =
        {
                &(*args->apps)->addedApps,
                &(*args->apps)->defaultApps,
                &(*args->apps)->removedApps,
                NULL,
                (ReadListEntry)_xdg_lst_group_read_entry_const
        };

        _xdg_app_read_list_file(args->buffer, file, &args2);
        fclose(file);
    }
}

XdgUserApps *xdg_user_defined_apps_load()
{
    assert(!_is_empty_list(&folders_list) && "Library was not initialized!");

    XdgUserApps *res = NULL;
    char buffer[READ_FROM_FILE_BUFFER_SIZE];
    InitFromHomeDirectoryArgs args = {buffer, &res};

    _xdg_for_each_home_data_dir((XdgDirectoryFunc)_xdg_load_user_defined_apps, &args);

    return res;
}

XdgList *xdg_user_defined_apps_lookup(XdgUserApps *apps, const char *mimeType, ListFileSections listFileSection, int create)
{
    assert(!_is_empty_list(&folders_list) && "Library was not initialized!");
    char buffer[MIME_TYPE_NAME_BUFFER_SIZE];
    char *sep;

    if (apps && (sep = strchr(mimeType, '/')) != NULL)
    {
        strncpy(buffer, mimeType, sep - mimeType);
        buffer[sep - mimeType] = 0;
        ++sep;

        AvlTree *tree;
        XdgMimeSubType *sub_type;

        switch (listFileSection)
        {
            case XdgAddedApplications:
                tree = &apps->addedApps;
                break;

            case XdgDefaultApplications:
                tree = &apps->defaultApps;
                break;

            case XdgRemovedApplications:
                tree = &apps->removedApps;
                break;

            default:
                return NULL;
        }

        if (create)
            return &_xdg_mime_sub_type_item_search_or_create(tree, buffer, sep)->apps.list;
        else
            if (sub_type = _xdg_mime_sub_type_item_search(tree, buffer, sep))
                return &sub_type->apps.list;
    }

    return NULL;
}

static void __xdg_user_defined_apps_save(const char *key, const XdgMimeSubType *mime_subtype, UserDefinedAppsArgs *args)
{
    XdgMimeSubTypeValue *item = (XdgMimeSubTypeValue *)mime_subtype->apps.list.head;

    if (item)
    {
        fprintf(args->file, "%s/%s=", args->type, key);

        do
            fprintf(args->file, "%s;", item->name);
        while (item = (XdgMimeSubTypeValue *)item->item.item.next);

        fwrite("\n", 1, 1, args->file);
    }
}

static void _xdg_user_defined_apps_save(const char *key, const XdgMimeType *mime_type, UserDefinedAppsArgs *args)
{
    args->type = key;
    depth_first_search(&mime_type->sub_types, (DepthFirstSearch)__xdg_user_defined_apps_save, args);
}

XdgListItem *xdg_user_defined_apps_head(XdgList *list)
{
    assert(list);
    return list->head;
}

XdgListItem *xdg_user_defined_apps_append(const XdgJointListItem *item, XdgList *list)
{
    assert(item && list);
    return (XdgListItem *)_xdg_list_app_item_append_copy(list, (XdgMimeSubTypeValue *)item);
}

XdgListItem *xdg_user_defined_apps_prepend(const XdgJointListItem *item, XdgList *list)
{
    assert(item && list);
    return (XdgListItem *)_xdg_list_app_item_prepend_copy(list, (XdgMimeSubTypeValue *)item);
}

void xdg_user_defined_apps_free(XdgUserApps *apps, int save)
{
    if (apps)
    {
        if (save)
        {
            FILE *file;

            if (file = fopen(apps->fileName, "w"))
            {
                UserDefinedAppsArgs args = { NULL, file };

                if (!is_empty_tree(&apps->addedApps))
                {
                    fwrite("[Added Associations]\n", strlen("[Added Associations]\n"), 1, file);
                    depth_first_search(&apps->addedApps, (DepthFirstSearch)_xdg_user_defined_apps_save, &args);
                    fwrite("\n", 1, 1, file);
                }

                if (!is_empty_tree(&apps->defaultApps))
                {
                    fwrite("[Default Applications]\n", strlen("[Default Applications]\n"), 1, file);
                    depth_first_search(&apps->defaultApps, (DepthFirstSearch)_xdg_user_defined_apps_save, &args);
                    fwrite("\n", 1, 1, file);
                }

                if (!is_empty_tree(&apps->removedApps))
                {
                    fwrite("[Removed Applications]\n", strlen("[Removed Applications]\n"), 1, file);
                    depth_first_search(&apps->removedApps, (DepthFirstSearch)_xdg_user_defined_apps_save, &args);
                    fwrite("\n", 1, 1, file);
                }

                fclose(file);
            }
        }

        clear_avl_tree_and_values(&apps->addedApps, (DestroyValue)_xdg_mime_type_map_item_free);
        clear_avl_tree_and_values(&apps->defaultApps, (DestroyValue)_xdg_mime_type_map_item_free);
        clear_avl_tree_and_values(&apps->removedApps, (DestroyValue)_xdg_mime_type_map_item_free);
        free(apps);
    }
}

char *xdg_app_icon_lookup(const XdgApp *app, const char *themeName, int size)
{
#ifdef THEMES_SPEC
	XdgAppGroup **group = (XdgAppGroup **)search_node(&app->groups->tree, "Desktop Entry");

	if (group)
	{
		XdgAppGroupEntry **entry = (XdgAppGroupEntry **)search_node(&(*group)->entries, "Icon");

		if (entry && !_is_empty_list(&(*entry)->values))
			return xdg_icon_lookup(((XdgValue *)(*entry)->values.head)->value, size, XdgThemeApplications, themeName);
	}
#endif

	return NULL;
}

const XdgAppGroup *xdg_app_group_lookup(const XdgApp *app, const char *group)
{
	if (app->groups)
	{
		XdgAppGroup **res = (XdgAppGroup **)search_node(&app->groups->tree, group);

		if (res)
			return (*res);
	}

	return NULL;
}

const XdgListItem *xdg_app_entry_lookup(const XdgAppGroup *group, const char *entry)
{
	XdgAppGroupEntry **res = (XdgAppGroupEntry **)search_node(&group->entries, entry);

	if (res)
		return (*res)->values.head;
	else
		return NULL;
}

const XdgListItem *xdg_app_localized_entry_lookup(const XdgAppGroup *group, const char *entry, const char *lang, const char *country, const char *modifier)
{
	XdgAppGroupEntry **value = (XdgAppGroupEntry **)search_node(&group->entries, entry);

	if (value && *value)
	{
        XdgList **res;
		char buffer[LOCALE_NAME_BUFFER_SIZE];

		if (lang && *lang)
			if (country && *country)
				if (modifier && *modifier)
				{
					strcpy(buffer, lang);
					strcat(buffer, "_");
					strcat(buffer, country);
					strcat(buffer, "@");
					strcat(buffer, modifier);

					res = (XdgList **)search_node(&(*value)->localized, buffer);

					if (res && !_is_empty_list(*res))
						return (*res)->head;
					else
					{
						(*strchr(buffer, '@')) = 0;

						res = (XdgList **)search_node(&(*value)->localized, buffer);

	                    if (res && !_is_empty_list(*res))
							return (*res)->head;
						else
						{
							(*strchr(buffer, '_')) = 0;
							strcat(buffer, "@");
							strcat(buffer, modifier);

							res = (XdgList **)search_node(&(*value)->localized, buffer);

		                    if (res && !_is_empty_list(*res))
								return (*res)->head;
							else
							{
								res = (XdgList **)search_node(&(*value)->localized, lang);

			                    if (res && !_is_empty_list(*res))
									return (*res)->head;
								else
									return (XdgListItem *)(*value)->values.head;
							}
						}
					}
				}
				else
				{
					strcpy(buffer, lang);
					strcat(buffer, "_");
					strcat(buffer, country);

					res = (XdgList **)search_node(&(*value)->localized, buffer);

                    if (res && !_is_empty_list(*res))
						return (*res)->head;
					else
					{
						res = (XdgList **)search_node(&(*value)->localized, lang);

						if (res && !_is_empty_list(*res))
							return (*res)->head;
						else
							return (XdgListItem *)(*value)->values.head;
					}
				}
			else
				if (modifier && *modifier)
				{
					strcpy(buffer, lang);
					strcat(buffer, "@");
					strcat(buffer, modifier);

					res = (XdgList **)search_node(&(*value)->localized, buffer);

					if (res && !_is_empty_list(*res))
						return (*res)->head;
					else
					{
						res = (XdgList **)search_node(&(*value)->localized, lang);

						if (res && !_is_empty_list(*res))
							return (*res)->head;
						else
							return (XdgListItem *)(*value)->values.head;
					}
				}
				else
				{
					res = (XdgList **)search_node(&(*value)->localized, lang);

					if (res && !_is_empty_list(*res))
						return (*res)->head;
					else
						return (XdgListItem *)(*value)->values.head;
				}

		return (XdgListItem *)(*value)->values.head;
	}

	return NULL;
}

const XdgApp *xdg_list_item_app(const XdgListItem *item)
{
    return ((XdgMimeSubTypeValue *)item)->app;
}

const XdgApp *xdg_joint_list_item_app(const XdgJointListItem *item)
{
	return ((XdgMimeSubTypeValue *)item)->app;
}

const char *xdg_joint_list_item_app_id(const XdgJointListItem *item)
{
	return ((XdgMimeSubTypeValue *)item)->name;
}

const char *xdg_list_item_app_group_entry_value(const XdgListItem *item)
{
	return ((XdgValue *)item)->value;
}
