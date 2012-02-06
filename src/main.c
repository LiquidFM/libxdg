#include "xdgapp.h"
#include <stdio.h>
#include <string.h>


int main(int argc, char *argv[])
{
	int res = xdg_app_rebuild_cache();

	if (res != 0)
		fprintf(stderr, "%s", strerror(res));

	return res;
}
