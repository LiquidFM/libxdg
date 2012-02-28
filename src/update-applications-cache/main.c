#include "../desktop/xdgapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void usage()
{
	fprintf(stdout,
			"Usage: update-applications-cache [DIRECTORY]..."
			"\n\n  Rebuilds the cache which contains \".desktop\" and \".list\" files"
			"\ndata in a given DIRECTORY."
			"\n\n  Basically DIRECTORY should be:"
			"\n\t~/.local/share/applications"
			"\n\t/usr/local/share/applications"
			"\n\t/usr/share/applications"
			"\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		usage();
	else
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
			usage();
		else
		{
			int i = 1;
			int res;

			for (; i < argc; ++i)
				if (res = xdg_app_rebuild_cache_file(argv[i]))
				{
					fprintf(stderr,
							"Failed to rebuild the cache in directory:\n\t%s\nError:\n\t%s\n",
							argv[i],
							strerror(res));

					return res;
				}
		}

	return EXIT_SUCCESS;
}
