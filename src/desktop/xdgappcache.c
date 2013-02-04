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

static void write_app_group_localized_entry(int fd, const XdgList *value)
{
    write(fd, value, sizeof(XdgList));

    XdgValue *item = (XdgValue *)value->head;

    while (item)
    {
        write(fd, item, sizeof(XdgValue) + strlen(item->value));
        item = (XdgValue *)item->item.next;
    }
}

static void write_app_group_entry(int fd, const XdgAppGroupEntry *value)
{
	write(fd, value, sizeof(XdgAppGroupEntry));

    XdgValue *item = (XdgValue *)value->values.head;

    while (item)
    {
        write(fd, item, sizeof(XdgValue) + strlen(item->value));
        item = (XdgValue *)item->item.next;
    }

	write_to_file(fd, &value->localized, write_app_key, (WriteValue)write_app_group_localized_entry, NULL);
}

static void write_app_group(int fd, const XdgAppGroup *value)
{
	write_to_file(fd, &value->entries, write_app_key, (WriteValue)write_app_group_entry, NULL);
}

void write_app(int fd, const XdgApp *value)
{
	if (value->groups && value->groups->owner == value)
	{
		write(fd, value, sizeof(XdgApp));
		write(fd, value->groups, sizeof(XdgAppGroups));
		write_to_file(fd, &value->groups->tree, write_app_key, (WriteValue)write_app_group, NULL);
	}
	else
	{
		XdgApp empty;
		memset(&empty, 0, sizeof(XdgApp));
		write(fd, &empty, sizeof(XdgApp));
	}
}

static void *read_app_group_localized_entry(void **memory)
{
    XdgList *list = (*memory);
    (*memory) += sizeof(XdgList);

    if (list->head)
    {
        XdgValue *tmp;
        XdgValue *prev = (*memory);

        list->head = (XdgListItem *)prev;
        prev->item.list = list;
        (*memory) += sizeof(XdgValue) + strlen(prev->value);

        while (prev->item.next)
        {
            tmp = (*memory);
            prev->item.next = (XdgListItem *)tmp;
            tmp->item.prev = (XdgListItem *)prev;
            tmp->item.list = list;
            prev = tmp;
            (*memory) += sizeof(XdgValue) + strlen(prev->value);
        }

        list->tail = (XdgListItem *)prev;
    }

    return list;
}

static void *read_app_group_entry(void **memory)
{
	XdgAppGroupEntry *res = (*memory);
	(*memory) += sizeof(XdgAppGroupEntry);

    if (res->values.head)
    {
        XdgValue *tmp;
        XdgValue *prev = (*memory);

        res->values.head = (XdgListItem *)prev;
        prev->item.list = &res->values;
        (*memory) += sizeof(XdgValue) + strlen(prev->value);

        while (prev->item.next)
        {
            tmp = (*memory);
            prev->item.next = (XdgListItem *)tmp;
            tmp->item.prev = (XdgListItem *)prev;
            tmp->item.list = &res->values;
            prev = tmp;
            (*memory) += sizeof(XdgValue) + strlen(prev->value);
        }

        res->values.tail = (XdgListItem *)prev;
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
	(*memory) += sizeof(XdgApp);

	if (value->groups)
	{
		value->groups = (*memory);
		(*memory) += sizeof(XdgAppGroups);

		value->groups->owner = value;
		memcpy(&value->groups->tree, map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_app_group, strcmp, NULL), sizeof(AvlTree));
	}

	return value;
}

static void write_mime_group_sub_type(int fd, const XdgMimeSubType *value)
{
	write(fd, value, sizeof(XdgMimeSubType));

    XdgMimeSubTypeValue *item = (XdgMimeSubTypeValue *)value->apps.list.head;

    while (item)
    {
        write(fd, item, sizeof(XdgMimeSubTypeValue) + strlen(item->name));
        item = (XdgMimeSubTypeValue *)item->item.item.next;
    }
}

void write_mime_group_type(int fd, const XdgMimeType *value)
{
	write_to_file(fd, &value->sub_types, write_app_key, (WriteValue)write_mime_group_sub_type, NULL);
}

static void *read_mime_group_sub_type(void **memory, const AvlTree *app_files_map)
{
    XdgApp **app;
	XdgMimeSubType *res = (*memory);
	(*memory) += sizeof(XdgMimeSubType);

    if (res->apps.list.head)
    {
        XdgMimeSubTypeValue *tmp;
        XdgMimeSubTypeValue *prev = (*memory);

        res->apps.list.head = (XdgListItem *)prev;
        prev->item.item.list = &res->apps.list;
        (*memory) += sizeof(XdgMimeSubTypeValue) + strlen(prev->name);

        if (app = (XdgApp **)search_node(app_files_map, prev->name))
            prev->app = (*app);
        else
            prev->app = NULL;

        while (prev->item.item.next)
        {
            tmp = (*memory);
            prev->item.item.next = (XdgListItem *)tmp;
            tmp->item.item.prev = (XdgListItem *)prev;
            tmp->item.item.list = &res->apps.list;

            if (app = (XdgApp **)search_node(app_files_map, tmp->name))
                tmp->app = (*app);
            else
                tmp->app = NULL;

            prev = tmp;
            (*memory) += sizeof(XdgMimeSubTypeValue) + strlen(prev->name);
        }

        res->apps.list.tail = (XdgListItem *)prev;
    }

	return res;
}

void *read_mime_group_type(void **memory, const AvlTree *app_files_map)
{
	XdgMimeType *value = (*memory);

	map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_mime_group_sub_type, strcmp, (void *)app_files_map);

	return value;
}

void write_file_watcher_list(int fd, const XdgList *list)
{
    write(fd, list, sizeof(XdgList));

    XdgFileWatcher *file = (XdgFileWatcher *)list->head;

    while (file)
    {
        write(fd, file, sizeof(XdgFileWatcher) + strlen(file->path));
        file = (XdgFileWatcher *)file->item.next;
    }
}

const XdgList *read_file_watcher_list(void **memory)
{
    XdgList *list = (*memory);
	(*memory) += sizeof(XdgList);

	if (list->head)
	{
	    XdgFileWatcher *tmp;
		XdgFileWatcher *prev = (*memory);

		list->head = (XdgListItem *)prev;
		prev->item.list = list;
        (*memory) += sizeof(XdgFileWatcher) + strlen(prev->path);

		while (prev->item.next)
		{
		    tmp = (*memory);
		    prev->item.next = (XdgListItem *)tmp;
		    tmp->item.prev = (XdgListItem *)prev;
		    tmp->item.list = list;
		    prev = tmp;
		    (*memory) += sizeof(XdgFileWatcher) + strlen(prev->path);
		}

		list->tail = (XdgListItem *)prev;
	}

	return list;
}
