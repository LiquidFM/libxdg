/* xdgmimearray.c: Private file.
 *
 * More info can be found at http://www.freedesktop.org/standards/
 *
 * Copyright (C) 2011  Dmitriy Vilkov <dav.daemon@gmail.com>
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

#include "xdgmimearray_p.h"
#include <stdlib.h>


void **_xdg_array_item_add(XdgArray *array, int alloc_granularity)
{
	void **res;

	if (array->capacity == 0)
		if (array->list == NULL)
		{
			array->list = malloc(alloc_granularity * sizeof(void *));
			array->capacity = alloc_granularity;
		}
		else
		{
			array->list = realloc(array->list, array->count * 2 * sizeof(void *));
			array->capacity = array->count;
		}

	res = &array->list[array->count];
	++array->count;
	--array->capacity;

	return res;
}

void _xdg_array_and_values_free(XdgArray *array)
{
	if (array->list)
	{
		int i = 0, size = array->count;

		for (; i < size; ++i)
			free(array->list[i]);

		free(array->list);
	}
}

void _xdg_array_free(XdgArray *array)
{
	free(array->list);
}

int xdg_mime_array_size(const XdgArray *array)
{
	return array->count;
}

const char *xdg_mime_array_string_item_at(const XdgArray *array, int index)
{
	return (char *)array->list[index];
}
