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
	const XdgFileWatcher *files;
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
	XdgFileWatcher *files;
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
struct XdgAppFolders
{
	XdgList list;
	XdgAppFolder app;
};
typedef struct XdgAppFolders XdgAppFolders;


static XdgAppFolders *folders_list = NULL;


/**
 * Memory allocation functions
 */
static void _xdg_list_value_item_add(XdgList **list, const char *line)
{
	XdgValue *value = malloc(sizeof(XdgValue) + strlen(line));

	_xdg_list_apped(list, (XdgList *)value);
	strcpy(value->value, line);
}

static void _xdg_list_value_item_free(XdgValue *list)
{
	_xdg_list_free((XdgList *)list, free);
}

static void _xdg_file_watcher_list_add(XdgList **list, const char *path, struct stat *st)
{
	XdgFileWatcher *res = malloc(sizeof(XdgFileWatcher) + strlen(path));

	_xdg_list_apped(list, (XdgList *)res);

	if (stat(path, st) == 0)
		res->mtime = st->st_mtime;
	else
		res->mtime = 0;

	strcpy(res->path, path);
}

static void _xdg_file_watcher_list_free(XdgFileWatcher *list)
{
	_xdg_list_free((XdgList *)list, free);
}

void _xdg_list_app_item_add(XdgList **list, const char *name, XdgApp *app)
{
	XdgMimeSubTypeValue *val = malloc(sizeof(XdgMimeSubTypeValue) + strlen(name));

	_xdg_list_apped(list, (XdgList *)val);

	val->app = app;
	strcpy(val->name, name);
}

static void _xdg_mime_sub_type_map_item_free(XdgMimeSubType *sub_type)
{
	_xdg_list_free((XdgList *)sub_type->apps, free);
	free(sub_type);
}

static void _xdg_mime_type_map_item_free(XdgMimeType *type)
{
	clear_avl_tree_and_values(&type->sub_types, (DestroyValue)_xdg_mime_sub_type_map_item_free);
	free(type);
}

static XdgAppGroupEntryValue *_xdg_app_group_entry_map_item_add(AvlTree *map, const char *name)
{
	XdgAppGroupEntryValue **res = (XdgAppGroupEntryValue **)search_or_create_node(map, name);

	if ((*res) == NULL)
	{
		(*res) = malloc(sizeof(XdgAppGroupEntryValue));
		memset(&(*res)->values, 0, sizeof(XdgList));
		init_avl_tree(&(*res)->localized, strdup, (DestroyKey)free, strcmp);
	}

	return (*res);
}

static void _xdg_app_group_entry_map_item_free(XdgAppGroupEntryValue *item)
{
	_xdg_list_value_item_free(item->values);
	clear_avl_tree_and_values(&item->localized, (DestroyValue)_xdg_list_value_item_free);
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
		cache->lst_files_map = map_from_memory(&memory, (ReadKey)read_app_key, (ReadValue)read_mime_group, strcmp, (void *)cache->app_files_map);

		return TRUE;
	}

	return FALSE;
}

static void _xdg_app_cache_write(XdgAppCahceFile *file, XdgAppData *data)
{
	write_version(file->fd, 1);
	write_file_watcher_list(file->fd, data->files);
	write_to_file(file->fd, &data->app_files_map, write_app_key, (WriteValue)write_app);
	write_to_file(file->fd, &data->asoc_map, write_app_key, (WriteValue)write_mime_group_type);
	write_to_file(file->fd, &data->lst_files_map, write_app_key, (WriteValue)write_mime_group);
}

static void _xdg_app_cache_free(XdgAppCache *cache)
{
	_xdg_app_cache_close(&cache->file);
}

static void _xdg_app_data_init(XdgAppData *data)
{
	init_avl_tree(&data->asoc_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&data->app_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&data->lst_files_map, strdup, (DestroyKey)free, strcmp);
	data->files = NULL;
}

static void _xdg_app_data_free(XdgAppData *data)
{
	clear_avl_tree_and_values(&data->asoc_map, (DestroyValue)_xdg_mime_type_map_item_free);
	clear_avl_tree_and_values(&data->app_files_map, (DestroyValue)_xdg_app_map_item_free);
	clear_avl_tree_and_values(&data->lst_files_map, (DestroyValue)_xdg_mime_group_map_item_free);
	_xdg_file_watcher_list_free(data->files);
}

static XdgAppFolders *_xdg_app_folders_new(XdgList **list)
{
	XdgAppFolders *res = malloc(sizeof(XdgAppFolders));

	_xdg_list_apped(list, (XdgList *)res);
	_xdg_app_data_init(&res->app.data);
	_xdg_app_cache_init(&res->app.cache);

	return res;
}

static void _xdg_app_folder_free(XdgAppFolders *folder)
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

static XdgMimeSubType *_xdg_mime_sub_type_item_search(const AvlTree *map, const char *name)
{
	char *sep;

	if ((sep = strchr(name, '/')) != NULL)
	{
		*sep = 0;
		XdgMimeType **type = (XdgMimeType **)search_node(map, name);
		*sep = '/';

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

	return NULL;
}

static void _xdg_app_group_read_entry_value(XdgList **list, char *line)
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

static void _xdg_app_group_read_mime_type_entry_value(XdgList **list, XdgApp *app, const char *app_name, AvlTree *asoc_map, char *line)
{
	char *sep;
	XdgMimeSubType *sub_type;

	for (sep = line; *sep && *sep != '\n'; ++sep)
		if (*sep == ';')
		{
			*sep = 0;
			_xdg_list_value_item_add(list, line);

			if (sub_type = _xdg_mime_sub_type_add(asoc_map, line))
				_xdg_list_app_item_add((XdgList **)&sub_type->apps, app_name, app);

			line = sep + 1;
		}

	if (*line != 0 && *line != '\n')
	{
		*sep = 0;
		_xdg_list_value_item_add(list, line);

		if (sub_type = _xdg_mime_sub_type_add(asoc_map, line))
			_xdg_list_app_item_add((XdgList **)&sub_type->apps, app_name, app);
	}
}

static void _xdg_app_group_read_entry(XdgAppGroup *group, XdgApp *app, const char *app_name, AvlTree *asoc_map, const char *line)
{
	char *sep;

	if ((sep = strchr(line, '=')) != NULL)
	{
		XdgList *values;
		char *start = sep + 1;
		REMOVE_WHITE_SPACES_LEFT(line, sep)
		REMOVE_WHITE_SPACES_RIGHT(start)

		if ((sep = strchr(line, '[')) != NULL)
		{
			*sep = 0;
			char *locale = sep + 1;
			XdgAppGroupEntryValue *entry = _xdg_app_group_entry_map_item_add(&group->entries, line);

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

				_xdg_app_group_read_entry_value((XdgList **)search_or_create_node(&entry->localized, buffer), start);
			}
			else
				_xdg_app_group_read_entry_value((XdgList **)&entry->values, start);
		}
		else
		{
			XdgAppGroupEntryValue *entry = _xdg_app_group_entry_map_item_add(&group->entries, line);

			if (strcmp(line, "MimeType") == 0)
				_xdg_app_group_read_mime_type_entry_value((XdgList **)&entry->values, app, app_name, asoc_map, start);
			else
				_xdg_app_group_read_entry_value((XdgList **)&entry->values, start);
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
		XdgMimeSubType *sub_type = _xdg_mime_sub_type_add(&group->types, line);

		if (sub_type)
		{
			for (sep = start; *sep && *sep != '\n'; ++sep)
				if (*sep == ';')
				{
					*sep = 0;
					_xdg_list_app_item_add((XdgList **)&sub_type->apps, start, _xdg_app_map_item_add(app_files_map, start));
					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_list_app_item_add((XdgList **)&sub_type->apps, start, _xdg_app_map_item_add(app_files_map, start));
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
					group = _xdg_app_group_map_item_add(&app->groups, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_app_group_read_entry(group, app, name, &data->asoc_map, buffer);
				else
					break;
}

static void _xdg_app_read_list_file(char *buffer, XdgAppData *data, FILE *file)
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
					group = _xdg_mime_group_map_item_add(&data->lst_files_map, buffer + 1);
				}
			}
			else
				if (group)
					_xdg_mime_group_read_entry(&data->app_files_map, group, buffer);
				else
					break;
}

static void __xdg_app_read_from_directory(char *buffer, XdgAppData *data, const char *directory_name, const char *preffix)
{
	DIR *dir;
	struct stat st;

	_xdg_file_watcher_list_add((XdgList **)&data->files, directory_name, &st);

	if (dir = opendir(directory_name))
	{
		FILE *file;
		char *file_name;
		struct dirent *entry;
		char *file_name_preffix;

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
					if (fnmatch("*.desktop", entry->d_name, FNM_NOESCAPE) != FNM_NOMATCH)
					{
						file_name = malloc(strlen(directory_name) + strlen(entry->d_name) + 2);
						strcpy(file_name, directory_name); strcat(file_name, "/"); strcat(file_name, entry->d_name);

						if (file = fopen(file_name, "r"))
						{
							_xdg_file_watcher_list_add((XdgList **)&data->files, file_name, &st);

							file_name_preffix = malloc(strlen(preffix) + strlen(entry->d_name) + 1);
							strcpy(file_name_preffix, preffix); strcat(file_name_preffix, entry->d_name);

							_xdg_app_read_desktop_file(buffer, data, file, file_name_preffix);

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
								_xdg_file_watcher_list_add((XdgList **)&data->files, file_name, &st);
								_xdg_app_read_list_file(buffer, data, file);
								fclose(file);
							}

							free(file_name);
						}

		closedir(dir);
	}
}

static void _xdg_app_read_from_directory(char *buffer, const char *directory_name, const char *preffix)
{
	char *file_name;
	XdgAppFolders *folder = _xdg_app_folders_new((XdgList **)&folders_list);

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

	__xdg_app_read_from_directory(buffer, &folder->app.data, directory_name, preffix);

	folder->app.asoc_map = &folder->app.data.asoc_map;
	folder->app.app_files_map = &folder->app.data.app_files_map;
	folder->app.lst_files_map = &folder->app.data.lst_files_map;
}

static int _init_from_directory(const char *directory, char *buffer)
{
	char *file_name;
	assert(directory != NULL);

	file_name = malloc (strlen (directory) + strlen ("/applications") + 1);
	strcpy (file_name, directory); strcat (file_name, "/applications");
	_xdg_app_read_from_directory(buffer, file_name, "");
	free (file_name);

	return FALSE; /* Keep processing */
}

void _xdg_app_init()
{
	char buffer[READ_FROM_FILE_BUFFER_SIZE];
	_xdg_for_each_data_dir((XdgDirectoryFunc)_init_from_directory, buffer);
}

void _xdg_app_shutdown()
{
	_xdg_list_free((XdgList *)folders_list, (XdgListItemFree)_xdg_app_folder_free);
	folders_list = NULL;
}

static BOOL _xdg_check_time_stamp(const XdgFileWatcher *files)
{
	struct stat st;
	XdgFileWatcher *next;
	XdgFileWatcher *file = (XdgFileWatcher *)files->list.head;

	while (file)
	{
		next = (XdgFileWatcher *)file->list.next;

		if (stat(file->path, &st) == 0)
		{
			if (st.st_mtime != file->mtime)
				return FALSE;
		}
		else
			if (file->mtime)
				return FALSE;

		file = next;
	}

	return TRUE;
}

int xdg_app_cache_file_is_valid()
{
	int res = FALSE;
//	XdgAppCache cache;
//
//	_xdg_app_cache_init(&cache);
//	_xdg_app_cache_new(&cache.file, APPLICATIONS_CACHE_FILE);
//
//	if (cache.file.error == 0)
//	{
//		void *memory = cache.file.memory;
//
//		if (read_version(&memory) == 1)
//		{
//			cache.files = read_file_watcher_list(&memory);
//			res = _xdg_check_time_stamp(cache.files);
//		}
//
//		_xdg_app_cache_close(&cache.file);
//	}

	return res;
}

int xdg_app_rebuild_cache_file()
{
	int res = 0;
//	XdgAppCache cache;
//
//	_xdg_app_cache_init(&cache);
//	_xdg_app_cache_new_empty(&cache.file, APPLICATIONS_CACHE_FILE);
//
//	if (cache.file.error == 0)
//	{
//		XdgAppData data;
//		char buffer[READ_FROM_FILE_BUFFER_SIZE];
//		InitFromDirectoryArgs args = {buffer, &data};
//
//		_xdg_app_data_init(&data);
//
//		_xdg_for_each_data_dir((XdgDirectoryFunc)_init_from_directory, &args);
//
//		_xdg_app_cache_write(&cache.file, &data);
//
//		_xdg_app_data_free(&data);
//	}
//	else
//		res = cache.file.error;
//
//	_xdg_app_cache_free(&cache);

	return res;
}

int xdg_app_cache_is_valid()
{
//	assert(folders_list);
//
//	if (folders_list->cache.files)
//		return _xdg_check_time_stamp(folders_list->cache.files);

	return FALSE;
}

int xdg_app_reload_cache()
{
//	assert(folders_list);
//
//	if (folders_list->cache.files)
//	{
//		XdgAppCache cache;
//
//		_xdg_app_cache_init(&cache);
//		_xdg_app_cache_new(&cache.file, APPLICATIONS_CACHE_FILE);
//
//		if (cache.file.error == 0)
//		{
//			_xdg_app_cache_read(&cache);
//			_xdg_app_cache_close(&folders_list->cache.file);
//			memcpy(&folders_list->cache, &cache, sizeof(XdgAppCache));
//
//			folders_list->asoc_map = folders_list->cache.asoc_map;
//			folders_list->app_files_map = folders_list->cache.app_files_map;
//			folders_list->lst_files_map = folders_list->cache.lst_files_map;
//
//			return TRUE;
//		}
//	}

	return FALSE;
}

static const XdgJointList *_xdg_apps_lookup(const char *mimeType, const char *group_name)
{
	assert(folders_list && "Library was not initialized!");
	XdgMimeSubType *sub_type;
	XdgMimeGroup **group;
	XdgList *item = (XdgList *)folders_list->list.head;
	XdgJointList *res = NULL;

	do
	{
		group = (XdgMimeGroup **)search_node(((XdgAppFolders *)item)->app.lst_files_map, group_name);

		if (group)
		{
			sub_type = _xdg_mime_sub_type_item_search(&(*group)->types, mimeType);

			if (sub_type)
				_xdg_joint_list_apped(&res, (XdgJointList *)sub_type->apps);
		}

		item = item->next;
	}
	while (item);

	return res;
}

const XdgJointList *xdg_default_apps_lookup(const char *mimeType)
{
	return _xdg_apps_lookup(mimeType, "Default Applications");
}

const XdgJointList *xdg_added_apps_lookup(const char *mimeType)
{
	return _xdg_apps_lookup(mimeType, "Added Associations");
}

const XdgJointList *xdg_removed_apps_lookup(const char *mimeType)
{
	return _xdg_apps_lookup(mimeType, "Removed Associations");
}

const XdgJointList *xdg_known_apps_lookup(const char *mimeType)
{
	assert(folders_list && "Library was not initialized!");
	XdgMimeSubType *sub_type;
	XdgList *item = (XdgList *)folders_list->list.head;
	XdgJointList *res = NULL;

	do
	{
		sub_type = _xdg_mime_sub_type_item_search(((XdgAppFolders *)item)->app.asoc_map, mimeType);

		if (sub_type)
			_xdg_joint_list_apped(&res, (XdgJointList *)sub_type->apps);

		item = item->next;
	}
	while (item);

	return res;
}

char *xdg_app_icon_lookup(const XdgApp *app, const char *themeName, int size)
{
#ifdef THEMES_SPEC
	XdgAppGroup **group = (XdgAppGroup **)search_node(&app->groups, "Desktop Entry");

	if (group)
	{
		XdgAppGroupEntryValue **entry = (XdgAppGroupEntryValue **)search_node(&(*group)->entries, "Icon");

		if (entry && (*entry)->values)
			return xdg_icon_lookup((*entry)->values->value, size, Applications, themeName);
	}
#endif

	return 0;
}

const XdgAppGroup *xdg_app_group_lookup(const XdgApp *app, const char *group)
{
	XdgAppGroup **res = (XdgAppGroup **)search_node(&app->groups, group);

	if (res)
		return (*res);
	else
		return 0;
}

const XdgList *xdg_app_entry_lookup(const XdgAppGroup *group, const char *entry)
{
	XdgAppGroupEntryValue **res = (XdgAppGroupEntryValue **)search_node(&group->entries, entry);

	if (res)
		return (XdgList *)&(*res)->values;
	else
		return 0;
}

const XdgList *xdg_app_localized_entry_lookup(const XdgAppGroup *group, const char *entry, const char *lang, const char *country, const char *modifier)
{
	XdgAppGroupEntryValue **value = (XdgAppGroupEntryValue **)search_node(&group->entries, entry);

	if (value && *value)
	{
		XdgValue **res;
		char buffer[LOCALE_NAME_BUFFER_SIZE];

		if (lang)
			if (country)
				if (modifier)
				{
					strcpy(buffer, lang);
					strcat(buffer, "_");
					strcat(buffer, country);
					strcat(buffer, "@");
					strcat(buffer, modifier);

					res = (XdgValue **)search_node(&(*value)->localized, buffer);

					if (res && *res)
						return (XdgList *)*res;
					else
					{
						(*strchr(buffer, '@')) = 0;

						res = (XdgValue **)search_node(&(*value)->localized, buffer);

						if (res && *res)
							return (XdgList *)*res;
						else
						{
							(*strchr(buffer, '_')) = 0;
							strcat(buffer, "@");
							strcat(buffer, modifier);

							res = (XdgValue **)search_node(&(*value)->localized, buffer);

							if (res && *res)
								return (XdgList *)*res;
							else
							{
								res = (XdgValue **)search_node(&(*value)->localized, lang);

								if (res && *res)
									return (XdgList *)*res;
								else
									return (XdgList *)(*value)->values;
							}
						}
					}
				}
				else
				{
					strcpy(buffer, lang);
					strcat(buffer, "_");
					strcat(buffer, country);

					res = (XdgValue **)search_node(&(*value)->localized, buffer);

					if (res && *res)
						return (XdgList *)*res;
					else
					{
						res = (XdgValue **)search_node(&(*value)->localized, lang);

						if (res && *res)
							return (XdgList *)*res;
						else
							return (XdgList *)(*value)->values;
					}
				}
			else
				if (modifier)
				{
					strcpy(buffer, lang);
					strcat(buffer, "@");
					strcat(buffer, modifier);

					res = (XdgValue **)search_node(&(*value)->localized, buffer);

					if (res && *res)
						return (XdgList *)*res;
					else
					{
						res = (XdgValue **)search_node(&(*value)->localized, lang);

						if (res && *res)
							return (XdgList *)*res;
						else
							return (XdgList *)(*value)->values;
					}
				}
				else
				{
					res = (XdgValue **)search_node(&(*value)->localized, lang);

					if (res && *res)
						return (XdgList *)*res;
					else
						return (XdgList *)(*value)->values;
				}

		return (XdgList *)(*value)->values;
	}

	return 0;
}

const XdgApp *xdg_list_item_app(const XdgJointList *list)
{
	return ((XdgMimeSubTypeValue *)list)->app;
}

const char *xdg_list_item_app_group_entry_value(const XdgList *list)
{
	return ((XdgValue *)list)->value;
}
