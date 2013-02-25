/** @internal @file xdg.c
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

#include "xdg.h"

#ifdef MIME_SPEC
#	include "mime/xdgmime_p.h"
#endif

#ifdef DESKTOP_SPEC
#	include "desktop/xdgapp_p.h"
#endif

#ifdef THEMES_SPEC
#	include "themes/xdgtheme_p.h"
#endif

#ifdef MENU_SPEC
#   include "menu/xdgmenu_p.h"
#endif


void xdg_init()
{
#ifdef MIME_SPEC
	_xdg_mime_init();
#endif

#ifdef DESKTOP_SPEC
	_xdg_app_init();
#endif

#ifdef THEMES_SPEC
	_xdg_themes_init();
#endif

#ifdef MENU_SPEC
	_xdg_menu_init();
#endif
}

void xdg_shutdown()
{
#ifdef MENU_SPEC
    _xdg_menu_shutdown();
#endif

#ifdef THEMES_SPEC
	_xdg_themes_shutdown();
#endif

#ifdef DESKTOP_SPEC
	_xdg_app_shutdown();
#endif

#ifdef MIME_SPEC
	_xdg_mime_shutdown();
#endif
}
