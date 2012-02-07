#include "xdgappcache_p.h"
#include "xdgmimedefs.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>


struct ReadAppGroupEntryData
{
	XdgApp *app;
	AvlTree *asoc_map;
	BOOL is_mime_type_entry;
};
typedef struct ReadAppGroupEntryData ReadAppGroupEntryData;


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

static void write_app_group_entry(int fd, const XdgAppGroupEntry *value)
{
	int i;

	write(fd, value, sizeof(XdgAppGroupEntry));
	write(fd, value->values.list, sizeof(void *) * value->values.count);

	for (i = 0; i < value->values.count; ++i)
		write(fd, value->values.list[i], strlen(value->values.list[i]) + 1);
}

static void write_app_group(int fd, const XdgAppGroup *value)
{
	write_to_file(fd, &value->entries, write_app_key, (WriteValue)write_app_group_entry);
}

void write_app(int fd, const XdgApp *value)
{
	write_to_file(fd, &value->groups, write_app_key, (WriteValue)write_app_group);
}

static char *read_app_group_entry_key(void **memory, ReadAppGroupEntryData *data)
{
	char *res = (*memory);

	(*memory) += strlen(res) + 1;

	data->is_mime_type_entry = (strcmp(res, "MimeType") == 0);

	return res;
}

static void *read_app_group_entry(void **memory, ReadAppGroupEntryData *data)
{
	int i;
	XdgAppGroupEntry *res = (*memory);
	(*memory) += sizeof(XdgAppGroupEntry);

	res->values.list = (*memory);
	(*memory) += sizeof(void *) * res->values.count;

	if (data->is_mime_type_entry)
	{
		XdgMimeSubType *sub_type;

		for (i = 0; i < res->values.count; ++i)
		{
			res->values.list[i] = (*memory);

			if ((sub_type = _xdg_mime_sub_type_add(data->asoc_map, (*memory))))
				_xdg_array_app_item_add(&sub_type->apps, "", data->app);

			(*memory) += strlen(*memory) + 1;
		}
	}
	else
		for (i = 0; i < res->values.count; ++i)
		{
			res->values.list[i] = (*memory);
			(*memory) += strlen(*memory) + 1;
		}

	return res;
}

static void *read_app_group(void **memory, ReadAppGroupEntryData *data)
{
	XdgAppGroup *value = (*memory);

	map_from_memory(memory, (ReadKey)read_app_group_entry_key, (ReadValue)read_app_group_entry, strcmp, data);

	return value;
}

void *read_app(void **memory, AvlTree *asoc_map)
{
	XdgApp *value = (*memory);
	ReadAppGroupEntryData data = {value, asoc_map, FALSE};

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
		XdgFileWatcher *file = list->head;

		while (file)
		{
			next = file->next;
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

	if (res->head)
	{
		XdgFileWatcher *prev = res;
		res->head = res;

		while ((res = (*memory))->head)
		{
			prev->next = res;
			res->head = prev->head;
			prev = res;

			(*memory) += sizeof(XdgFileWatcher) + strlen(res->path);
		}

		(*memory) += sizeof(XdgFileWatcher);

		return prev;
	}

	return NULL;
}
