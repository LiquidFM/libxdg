#include "xdgappcache_p.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>


XdgAppCahce *_xdg_app_cache_new_empty(const char *file_name)
{
	XdgAppCahce cache = {open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), MAP_FAILED, 0};

	if (cache.fd >= 0)
	{
		struct stat st;

		if (fstat(cache.fd, &st) == 0)
		{
			XdgAppCahce *res = malloc(sizeof(XdgAppCahce));

			cache.size = st.st_size;
			memcpy(res, &cache, sizeof(XdgAppCahce));

			return res;
		}

		close(cache.fd);
	}

	return NULL;
}

XdgAppCahce *_xdg_app_cache_new(const char *file_name)
{
	XdgAppCahce cache = {open(file_name, O_RDONLY, 0), MAP_FAILED, 0};

	if (cache.fd >= 0)
	{
		struct stat st;

		if (fstat(cache.fd, &st) == 0)
		{
			cache.size = st.st_size;
			cache.memory = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, cache.fd, 0);

			if (cache.memory != MAP_FAILED)
			{
				XdgAppCahce *res = malloc(sizeof(XdgAppCahce));
				memcpy(res, &cache, sizeof(XdgAppCahce));

				return res;
			}
		}

		close(cache.fd);
	}

	return NULL;
}

void _xdg_app_cache_free(XdgAppCahce *cache)
{
	if (cache->fd >= 0)
	{
		if (cache->memory != MAP_FAILED)
		      munmap(cache->memory, cache->size);

		close(cache->fd);
	}

	free(cache);
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

static void *read_app_group_entry(void **memory)
{
	int i;
	XdgAppGroupEntry *value = (*memory);
	(*memory) += sizeof(XdgAppGroupEntry);

	value->values.list = (*memory);
	(*memory) += sizeof(void *) * value->values.count;

	for (i = 0; i < value->values.count; ++i)
	{
		value->values.list[i] = (*memory);
		(*memory) += strlen(*memory) + 1;
	}

	return value;
}

static void *read_app_group(void **memory)
{
	XdgAppGroup *value = (*memory);

	map_from_memory(memory, read_app_key, read_app_group_entry, strcmp);

	return value;
}

void *read_app(void **memory)
{
	XdgApp *value = (*memory);

	map_from_memory(memory, read_app_key, read_app_group, strcmp);

	return value;
}

void write_list_mime_group(int fd, const XdgMimeGroup *value)
{

}

void *read_list_mime_group(void **memory)
{
	return NULL;
}

void write_mime_type(int fd, const XdgMimeType *value)
{

}

void *read_mime_type(void **memory)
{
	return NULL;
}
