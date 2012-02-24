/** @internal @file xdgappcache.c
 *  @brief Private file.
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

#include "xdgappcache_p.h"
#include "xdgmimedefs.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>


void _xdg_app_cache_new_empty(XdgAppCahceFile *cache, const char *file_name)
{
	cache->error = 0;
	cache->fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	cache->memory = MAP_FAILED;
	cache->size = 0;

	if (cache->fd == -1)
		cache->error = errno;
}

void _xdg_app_cache_new(XdgAppCahceFile *cache, const char *file_name)
{
	cache->error = 0;
	cache->fd = open(file_name, O_RDONLY, 0);
	cache->memory = MAP_FAILED;
	cache->size = 0;

	if (cache->fd >= 0)
	{
		struct stat st;

		if (fstat(cache->fd, &st) == 0)
		{
			cache->size = st.st_size;
			cache->memory = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, cache->fd, 0);

			if (cache->memory == MAP_FAILED)
				cache->error = errno;
			else
			{
				madvise(cache->memory, cache->size, POSIX_MADV_SEQUENTIAL);
				return;
			}
		}
		else
			cache->error = errno;

		close(cache->fd);
	}
	else
		cache->error = errno;
}

void _xdg_app_cache_close(XdgAppCahceFile *cache)
{
	if (cache->fd >= 0)
	{
		if (cache->memory != MAP_FAILED)
		      munmap(cache->memory, cache->size);

		close(cache->fd);
		cache->fd = -1;
	}
}

void write_version(int fd, xdg_uint32_t version)
{
	write(fd, &version, sizeof(xdg_uint32_t));
}

xdg_uint32_t read_version(void **memory)
{
	xdg_uint32_t version = (*(xdg_uint32_t *)(*memory));

	(*memory) += sizeof(xdg_uint32_t);

	return version;
}

void write_app_key(int fd, const char *key)
{
	write(fd, key, strlen(key) + 1);
}

char *read_app_key(void **memory)
{
	char *res = (*memory);

	(*memory) += strlen(res) + 1;

	return res;
}

static void write_app_group_localized_entry(int fd, const XdgValue *value)
{
	XdgValue empty;
	memset(&empty, 0, sizeof(XdgValue));

	if (value)
	{
		XdgValue *item = (XdgValue *)value->list.head;

		while (item)
		{
			write(fd, item, sizeof(XdgValue) + strlen(item->value));
			item = (XdgValue *)item->list.next;
		}
	}

	write(fd, &empty, sizeof(XdgValue));
}

static void write_app_group_entry(int fd, const XdgAppGroupEntryValue *value)
{
	write(fd, value, sizeof(XdgAppGroupEntryValue));

	if (value->values)
	{
		XdgValue *item = (XdgValue *)value->values->list.head;

		XdgValue empty;
		memset(&empty, 0, sizeof(XdgValue));

		while (item)
		{
			write(fd, item, sizeof(XdgValue) + strlen(item->value));
			item = (XdgValue *)item->list.next;
		}

		write(fd, &empty, sizeof(XdgValue));
	}

	write_to_file(fd, &value->localized, write_app_key, (WriteValue)write_app_group_localized_entry);
}

static void write_app_group(int fd, const XdgAppGroup *value)
{
	write_to_file(fd, &value->entries, write_app_key, (WriteValue)write_app_group_entry);
}

void write_app(int fd, const XdgApp *value)
{
	write_to_file(fd, &value->groups, write_app_key, (WriteValue)write_app_group);
}

static void *read_app_group_localized_entry(void **memory)
{
	XdgValue *res = (*memory);
	(*memory) += sizeof(XdgValue) + strlen(res->value);

	if (res->list.head)
	{
		XdgValue *prev = res;
		res->list.head = (XdgList *)res;

		while ((res = (*memory))->list.head)
		{
			prev->list.next = (XdgList *)res;
			res->list.head = prev->list.head;
			prev = res;

			(*memory) += sizeof(XdgValue) + strlen(res->value);
		}

		(*memory) += sizeof(XdgValue);

		return prev;
	}

	return NULL;
}

static void *read_app_group_entry(void **memory)
{
	XdgAppGroupEntryValue *res = (*memory);
	(*memory) += sizeof(XdgAppGroupEntryValue);

	if (res->values)
	{
		XdgValue *prev = res->values = (*memory);
		res->values->list.head = (XdgList *)res->values;
		(*memory) += sizeof(XdgValue) + strlen(res->values->value);

		while ((res->values = (*memory))->list.head)
		{
			prev->list.next = (XdgList *)res->values;
			res->values->list.head = prev->list.head;
			prev = res->values;

			(*memory) += sizeof(XdgValue) + strlen(res->values->value);
		}

		(*memory) += sizeof(XdgValue);

		res->values = prev;
	}

	memcpy(&res->localized, map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_app_group_localized_entry, strcmp, NULL), sizeof(AvlTree));

	return res;
}

static void *read_app_group(void **memory)
{
	XdgAppGroup *value = (*memory);

	map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_app_group_entry, strcmp, NULL);

	return value;
}

void *read_app(void **memory)
{
	XdgApp *value = (*memory);

	map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_app_group, strcmp, NULL);

	return value;
}

static void write_mime_group_sub_type(int fd, const XdgMimeSubType *value)
{
	write(fd, value, sizeof(XdgMimeSubType));

	if (value->apps)
	{
		XdgMimeSubTypeValue *item = (XdgMimeSubTypeValue *)value->apps->list.list.head;

		XdgMimeSubTypeValue empty;
		memset(&empty, 0, sizeof(XdgMimeSubTypeValue));

		while (item)
		{
			write(fd, item, sizeof(XdgMimeSubTypeValue) + strlen(item->name));
			item = (XdgMimeSubTypeValue *)item->list.list.next;
		}

		write(fd, &empty, sizeof(XdgMimeSubTypeValue));
	}
}

void write_mime_group_type(int fd, const XdgMimeType *value)
{
	write_to_file(fd, &value->sub_types, write_app_key, (WriteValue)write_mime_group_sub_type);
}

void write_mime_group(int fd, const XdgMimeGroup *value)
{
	write_to_file(fd, &value->types, write_app_key, (WriteValue)write_mime_group_type);
}

static void *read_mime_group_sub_type(void **memory, const AvlTree *app_files_map)
{
	XdgMimeSubType *res = (*memory);
	(*memory) += sizeof(XdgMimeSubType);

	if (res->apps)
	{
		XdgApp **app;
		XdgMimeSubTypeValue *prev;

		res->apps = (*memory);

		if (app = (XdgApp **)search_node(app_files_map, res->apps->name))
			res->apps->app = (*app);
		else
			res->apps->app = 0;

		(*memory) += sizeof(XdgMimeSubTypeValue) + strlen(res->apps->name);

		prev = (XdgMimeSubTypeValue *)res->apps;
		res->apps->list.list.head = (XdgList *)res->apps;

		while ((res->apps = (*memory))->list.head)
		{
			prev->list.list.next = (XdgList *)res->apps;
			res->apps->list.head = prev->list.head;
			prev = res->apps;

			if (app = (XdgApp **)search_node(app_files_map, res->apps->name))
				res->apps->app = (*app);
			else
				res->apps->app = 0;

			(*memory) += sizeof(XdgMimeSubTypeValue) + strlen(res->apps->name);
		}

		(*memory) += sizeof(XdgMimeSubTypeValue);

		res->apps = prev;
	}

	return res;
}

void *read_mime_group_type(void **memory, const AvlTree *app_files_map)
{
	XdgMimeType *value = (*memory);

	map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_mime_group_sub_type, strcmp, (void *)app_files_map);

	return value;
}

void *read_mime_group(void **memory, const AvlTree *app_files_map)
{
	XdgMimeGroup *value = (*memory);

	map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_mime_group_type, strcmp, (void *)app_files_map);

	return value;
}

void write_file_watcher_list(int fd, const XdgFileWatcher *list)
{
	XdgFileWatcher empty;
	memset(&empty, 0, sizeof(XdgFileWatcher));

	if (list)
	{
		XdgFileWatcher *file = (XdgFileWatcher *)list->list.head;

		while (file)
		{
			write(fd, file, sizeof(XdgFileWatcher) + strlen(file->path));
			file = (XdgFileWatcher *)file->list.next;
		}
	}

	write(fd, &empty, sizeof(XdgFileWatcher));
}

const XdgFileWatcher *read_file_watcher_list(void **memory)
{
	XdgFileWatcher *res = (*memory);
	(*memory) += sizeof(XdgFileWatcher) + strlen(res->path);

	if (res->list.head)
	{
		XdgFileWatcher *prev = res;
		res->list.head = (XdgList *)res;

		while ((res = (*memory))->list.head)
		{
			prev->list.next = (XdgList *)res;
			res->list.head = prev->list.head;
			prev = res;

			(*memory) += sizeof(XdgFileWatcher) + strlen(res->path);
		}

		(*memory) += sizeof(XdgFileWatcher);

		return prev;
	}

	return NULL;
}
