#include "xdgappcache_p.h"
#include "xdgmimedefs.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>


struct ReadAppGroupEntryArgs
{
	XdgApp *app;
	AvlTree *asoc_map;
	BOOL is_mime_type_entry;
};
typedef struct ReadAppGroupEntryArgs ReadAppGroupEntryArgs;


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
				return;
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
	}
}

void write_version(int fd, int version)
{
	write(fd, &version, sizeof(int));
}

int read_version(void **memory)
{
	int version = (*(int *)(*memory));

	(*memory) += sizeof(int);

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

static void write_app_group_localized_entry(int fd, const XdgEncodedValue *value)
{
	XdgEncodedValue empty;
	memset(&empty, 0, sizeof(XdgEncodedValue));

	if (value)
	{
		XdgEncodedValue *next;
		XdgEncodedValue *item = (XdgEncodedValue *)value->list.head;

		while (item)
		{
			next = (XdgEncodedValue *)item->list.next;
			write(fd, item, sizeof(XdgEncodedValue) + strlen(item->encoding) + strlen(item->value));
			item = next;
		}
	}

	write(fd, &empty, sizeof(XdgEncodedValue));
}

static void write_app_group_entry(int fd, const XdgAppGroupEntryValue *value)
{
	write(fd, value, sizeof(XdgAppGroupEntryValue));

	if (value->values)
	{
		XdgValue *next;
		XdgValue *item = (XdgValue *)value->values->list.head;

		XdgValue empty;
		memset(&empty, 0, sizeof(XdgValue));

		while (item)
		{
			next = (XdgValue *)item->list.next;
			write(fd, item, sizeof(XdgValue) + strlen(item->value));
			item = next;
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

static char *read_app_group_entry_key(void **memory, ReadAppGroupEntryArgs *data)
{
	char *res = (*memory);

	(*memory) += strlen(res) + 1;

	data->is_mime_type_entry = (strcmp(res, "MimeType") == 0);

	return res;
}

static void *read_app_group_localized_entry(void **memory, ReadAppGroupEntryArgs *data)
{
	XdgEncodedValue *res = (*memory);
	res->encoding = res->data;
	res->value = res->data + strlen(res->encoding) + 1;
	(*memory) += sizeof(XdgEncodedValue) + strlen(res->encoding) + strlen(res->value);

	if (res->list.head)
	{
		XdgEncodedValue *prev = res;
		res->list.head = (XdgList *)res;

		while ((res = (*memory))->list.head)
		{
			prev->list.next = (XdgList *)res;
			res->list.head = prev->list.head;
			prev = res;

			res->encoding = res->data;
			res->value = res->data + strlen(res->encoding) + 1;
			(*memory) += sizeof(XdgEncodedValue) + strlen(res->encoding) + strlen(res->value);
		}

		(*memory) += sizeof(XdgEncodedValue);

		return prev;
	}

	return NULL;
}

static void *read_app_group_entry(void **memory, ReadAppGroupEntryArgs *data)
{
	XdgAppGroupEntryValue *res = (*memory);
	(*memory) += sizeof(XdgAppGroupEntryValue);

	if (res->values)
	{
		XdgValue *prev = res->values = (*memory);
		res->values->list.head = (XdgList *)res->values;
		(*memory) += sizeof(XdgValue) + strlen(res->values->value);

		if (data->is_mime_type_entry)
		{
			XdgMimeSubType *sub_type;

			if ((sub_type = _xdg_mime_sub_type_add(data->asoc_map, res->values->value)))
				_xdg_array_app_item_add(&sub_type->apps, "", data->app);

			while ((res->values = (*memory))->list.head)
			{
				prev->list.next = (XdgList *)res->values;
				res->values->list.head = prev->list.head;
				prev = res->values;

				if ((sub_type = _xdg_mime_sub_type_add(data->asoc_map, res->values->value)))
					_xdg_array_app_item_add(&sub_type->apps, "", data->app);

				(*memory) += sizeof(XdgValue) + strlen(res->values->value);
			}
		}
		else
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

	memcpy(&res->localized, map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_app_group_localized_entry, strcmp, data), sizeof(AvlTree));

	return res;
}

static void *read_app_group(void **memory, ReadAppGroupEntryArgs *data)
{
	XdgAppGroup *value = (*memory);

	map_from_memory(memory, (ReadKey)read_app_group_entry_key, (ReadValue)read_app_group_entry, strcmp, data);

	return value;
}

void *read_app(void **memory, AvlTree *asoc_map)
{
	XdgApp *value = (*memory);
	ReadAppGroupEntryArgs data = {value, asoc_map, FALSE};

	map_from_memory(memory, (ReadKey)read_app_key, (ReadValue)read_app_group, strcmp, &data);

	return value;
}

static void write_mime_group_sub_type(int fd, const XdgMimeSubType *value)
{
	int i;

	write(fd, value, sizeof(XdgMimeSubType));
	write(fd, value->apps.list, sizeof(void *) * value->apps.count);

	for (i = 0; i < value->apps.count; ++i)
		write(fd, value->apps.list[i], sizeof(XdgMimeSubTypeValue) + strlen(((XdgMimeSubTypeValue *)value->apps.list[i])->name));
}

static void write_mime_group_type(int fd, const XdgMimeType *value)
{
	write_to_file(fd, &value->sub_types, write_app_key, (WriteValue)write_mime_group_sub_type);
}

void write_mime_group(int fd, const XdgMimeGroup *value)
{
	write_to_file(fd, &value->types, write_app_key, (WriteValue)write_mime_group_type);
}

static void *read_mime_group_sub_type(void **memory, const AvlTree *app_files_map)
{
	int i;
	XdgApp **app;
	XdgMimeSubTypeValue *value;

	XdgMimeSubType *res = (*memory);
	(*memory) += sizeof(XdgMimeSubType);

	res->apps.list = (*memory);
	(*memory) += sizeof(void *) * res->apps.count;

	for (i = 0; i < res->apps.count; ++i)
	{
		res->apps.list[i] = value = (*memory);

		if (app = (XdgApp **)search_node(app_files_map, value->name))
			value->app = (*app);
		else
			value->app = 0;

		(*memory) += sizeof(XdgMimeSubTypeValue) + strlen(value->name);
	}

	return res;
}

static void *read_mime_group_type(void **memory, const AvlTree *app_files_map)
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
		XdgFileWatcher *next;
		XdgFileWatcher *file = (XdgFileWatcher *)list->list.head;

		while (file)
		{
			next = (XdgFileWatcher *)file->list.next;
			write(fd, file, sizeof(XdgFileWatcher) + strlen(file->path));
			file = next;
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
