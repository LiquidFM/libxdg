/* xdgmimeapp.c: Private file.  Datastructure for storing
 * the .desktop files.
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

#include "xdgapp_p.h"
#include "xdgappcache_p.h"
#include "xdgmimedefs.h"
#include "xdgbasedirectory.h"
#include "xdgtheme.h"
#include "avltree.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <assert.h>


#define APPLICATIONS_CACHE_FILE "/usr/share/applications/applications.cache"


/**
 * Main data structures
 */
struct XdgAppCahce
{
	XdgAppCahceFile file;
	const XdgFileWatcher *files;
	const AvlTree *app_files_map;
	const AvlTree *lst_files_map;
	AvlTree asoc_map;
};
typedef struct XdgAppCahce XdgAppCahce;


struct XdgApplications
{
	XdgAppCahce cache;
	AvlTree app_files_map;
	AvlTree lst_files_map;
	AvlTree asoc_map;
	XdgFileWatcher *files;
};
typedef struct XdgApplications XdgApplications;


struct InitFromDirectoryData
{
	char *buffer;
	XdgApplications *applications;

};
typedef struct InitFromDirectoryData InitFromDirectoryData;


static XdgApplications *applications_list = NULL;



/**
 * Memory allocation functions
 *
 */
static void _xdg_file_watcher_list_add(XdgFileWatcher **list, const char *path, struct stat *st)
{
	XdgFileWatcher *res = malloc(sizeof(XdgFileWatcher) + strlen(path));

	if (*list)
	{
		res->head = (*list)->head;
		res->next = NULL;
		(*list)->next = res;
		(*list) = res;
	}
	else
	{
		(*list) = res;
		res->head = res;
		res->next = NULL;
	}

	if (stat(path, st) == 0)
		res->mtime = st->st_mtime;
	else
		res->mtime = 0;

	strcpy(res->path, path);
}

static void _xdg_file_watcher_list_free(XdgFileWatcher *list)
{
	if (list)
	{
		XdgFileWatcher *next;
		XdgFileWatcher *file = list->head;

		while (file)
		{
			next = file->next;
			free(file);
			file = next;
		}
	}
}

static void _xdg_array_string_item_add(XdgArray *array, const char *name)
{
	(*_xdg_array_item_add(array, 2)) = strdup(name);
}

static void _xdg_array_app_item_add(XdgArray *array, XdgApp *app)
{
	(*_xdg_array_item_add(array, 2)) = app;
}

static void _xdg_mime_sub_type_map_item_free(XdgMimeSubType *sub_type)
{
	_xdg_array_free(&sub_type->apps);
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

XdgMimeSubType *xdg_mime_sub_type_add(AvlTree *map, const char *name)
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

					if (sub_type = xdg_mime_sub_type_add(asoc_map, start))
						_xdg_array_app_item_add(&sub_type->apps, app);

					start = sep + 1;
				}

			if (*start != 0 && *start != '\n')
			{
				*sep = 0;
				_xdg_array_string_item_add(&entry->values, start);

				if (sub_type = xdg_mime_sub_type_add(asoc_map, start))
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
		XdgMimeSubType *sub_type = xdg_mime_sub_type_add(&group->types, line);

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

static void _xdg_applications_read_from_directory(char *buffer, XdgApplications *applications, const char *directory_name, const char *preffix)
{
	DIR *dir;
	struct stat st;

	_xdg_file_watcher_list_add(&applications->files, directory_name, &st);

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

					_xdg_applications_read_from_directory(buffer, applications, file_name, file_name_preffix);

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
							_xdg_file_watcher_list_add(&applications->files, file_name, &st);

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
								_xdg_file_watcher_list_add(&applications->files, file_name, &st);
								_xdg_applications_read_list_file(buffer, applications, file);
								fclose(file);
							}

							free(file_name);
						}

		closedir(dir);
	}
}

static int _init_from_directory(const char *directory, InitFromDirectoryData *user_data)
{
	char *file_name;
	assert(directory != NULL);

	file_name = malloc (strlen (directory) + strlen ("/applications") + 1);
	strcpy (file_name, directory); strcat (file_name, "/applications");
	_xdg_applications_read_from_directory(user_data->buffer, user_data->applications, file_name, "");
	free (file_name);

	return FALSE; /* Keep processing */
}

static void xdg_app_cache_init(XdgAppCahce *cache)
{
	_xdg_app_cache_new(&cache->file, APPLICATIONS_CACHE_FILE);
	init_avl_tree(&cache->asoc_map, strdup, (DestroyKey)free, strcmp);
	cache->files = NULL;
}

static void xdg_app_cache_free(XdgAppCahce *cache)
{
	clear_avl_tree_and_values(&cache->asoc_map, (DestroyValue)_xdg_mime_type_map_item_free);
	_xdg_app_cache_close(&cache->file);
}

static XdgApplications *_xdg_applications_new(void)
{
	XdgApplications *res = malloc(sizeof(XdgApplications));

	xdg_app_cache_init(&res->cache);
	init_avl_tree(&res->app_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&res->lst_files_map, strdup, (DestroyKey)free, strcmp);
	init_avl_tree(&res->asoc_map, strdup, (DestroyKey)free, strcmp);
	res->files = NULL;

	return res;
}

static void _xdg_applications_free(XdgApplications *applications)
{
	_xdg_file_watcher_list_free(applications->files);
	clear_avl_tree_and_values(&applications->app_files_map, (DestroyValue)_xdg_app_map_item_free);
	clear_avl_tree_and_values(&applications->lst_files_map, (DestroyValue)_xdg_mime_group_map_item_free);
	clear_avl_tree_and_values(&applications->asoc_map, (DestroyValue)_xdg_mime_type_map_item_free);
	xdg_app_cache_free(&applications->cache);
	free(applications);
}

void _xdg_app_init()
{
	char buffer[READ_FROM_FILE_BUFFER_SIZE];
	InitFromDirectoryData data = {buffer, applications_list = _xdg_applications_new()};

	_xdg_for_each_data_dir((XdgDirectoryFunc)_init_from_directory, &data);
}

void _xdg_app_shutdown()
{
	if (applications_list)
	{
		_xdg_applications_free(applications_list);
		applications_list = NULL;
	}
}

void xdg_app_rebuild_cache()
{
	XdgAppCahceFile cache;

	_xdg_app_cache_new_empty(&cache, "/home/dav/app.cache");

	if (cache.error == 0)
	{
		write_file_watcher_list(cache.fd, applications_list->files);
		write_to_file(cache.fd, &applications_list->app_files_map, write_app_key, (WriteValue)write_app);
		write_to_file(cache.fd, &applications_list->lst_files_map, write_app_key, (WriteValue)write_list_mime_group);
		_xdg_app_cache_close(&cache);
	}

	_xdg_app_cache_new(&cache, "/home/dav/app.cache");

	if (cache.error == 0)
	{
		AvlTree asoc_map;
		init_avl_tree(&asoc_map, strdup, (DestroyKey)free, strcmp);

		void *memory = cache.memory;
		read_file_watcher_list(&memory);
		const AvlTree *app_files_map = map_from_memory(&memory, (ReadKey)read_app_key, (ReadValue)read_app, strcmp, &asoc_map);

		clear_avl_tree_and_values(&asoc_map, (DestroyValue)_xdg_mime_type_map_item_free);

		XdgApp **app = (XdgApp **)search_node(app_files_map, "kde4-ark.desktop");
		XdgAppGroup **group = (XdgAppGroup **)search_node(&(*app)->groups, "Desktop Entry");

		_xdg_app_cache_close(&cache);
	}
}

const XdgArray *xdg_default_apps_lookup(const char *mimeType)
{
	XdgMimeGroup **group = (XdgMimeGroup **)search_node(&applications_list->lst_files_map, "Default Applications");

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

const XdgArray *xdg_user_apps_lookup(const char *mimeType)
{
	XdgMimeGroup **group = (XdgMimeGroup **)search_node(&applications_list->lst_files_map, "Added Associations");

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

const XdgArray *xdg_known_apps_lookup(const char *mimeType)
{
	char mimeTypeCopy[MIME_TYPE_NAME_BUFFER_SIZE];
	strncpy(mimeTypeCopy, mimeType, MIME_TYPE_NAME_BUFFER_SIZE);
	XdgMimeSubType *sub_type = _xdg_mime_sub_type_item_search(&applications_list->asoc_map, mimeTypeCopy);

	if (sub_type)
		return &sub_type->apps;
	else
		return 0;
}

char *xdg_app_icon_lookup(const XdgApp *app, const char *themeName, int size)
{
	XdgAppGroup **group = (XdgAppGroup **)search_node(&app->groups, "Desktop Entry");

	if (group)
	{
		XdgAppGroupEntry **entry = (XdgAppGroupEntry **)search_node(&(*group)->entries, "Icon");

		if (entry && (*entry)->values.count)
			return xdg_icon_lookup((*entry)->values.list[0], size, Applications, themeName);
	}

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

const XdgArray *xdg_app_entry_lookup(const XdgAppGroup *group, const char *entry)
{
	XdgAppGroupEntry **res = (XdgAppGroupEntry **)search_node(&group->entries, entry);

	if (res)
		return &(*res)->values;
	else
		return 0;
}

const XdgApp *xdg_array_app_item_at(const XdgArray *array, int index)
{
	return (XdgApp *)array->list[index];
}
