/* xdgmimetheme_p.h: Private file.
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

#ifndef __XDG_MIME_THEME_P_H_
#define __XDG_MIME_THEME_P_H_

#include "xdgmimetheme.h"


typedef struct XdgThemes XdgThemes;

XdgThemes *_xdg_mime_themes_new(void);
void _xdg_mime_themes_read_from_directory(XdgThemes *themes);
void _xdg_mime_themes_free(XdgThemes *themes);

char *_xdg_mime_type_icon_lookup(XdgThemes *themes, const char *mime, int size, const char *theme);
char *_xdg_mime_icon_lookup(XdgThemes *themes, const char *icon, int size, Context context, const char *theme);

#endif /* __XDG_MIME_THEME_P_H_ */
