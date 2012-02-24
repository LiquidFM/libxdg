#include "../desktop/xdgapp.h"
#include <stdio.h>
#include <string.h>


int main(int argc, char *argv[])
{
	RebuildCacheResult result = {0, NULL};

	xdg_app_rebuild_cache_in_each_data_dir(&result);

	if (result.error != 0)
	{
		fprintf(stderr,
				"Failed to rebuild the cache in directory:\n\t%s\nError:\n\t%s\n",
				result.directory,
				strerror(result.error));

		free(result.directory);
	}

	return result.error;
}
