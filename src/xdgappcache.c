#include "xdgappcache_p.h"
#include <fcntl.h>
#include <string.h>


XdgAppCahce *_xdg_app_cache_new_empty(const char *file_name)
{
	XdgAppCahce cache = {open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0), NULL, 0};

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

void _xdg_app_cache_free(XdgAppCahce *cache)
{
	if (cache->fd >= 0)
		close(cache->fd);

	free(cache);
}
