/** @file xdg.h
 *  @brief Main header file.
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

#ifndef XDG_H_
#define XDG_H_

#ifdef HAVE_CONFIG_H
#	include <xdg/config.h>
#endif

#ifdef MIME_SPEC
#	include "mime/xdgmime.h"
#endif

#ifdef DESKTOP_SPEC
#	include "desktop/xdgapp.h"
#endif

#ifdef THEMES_SPEC
#	include "themes/xdgtheme.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize the library.
 *
 * @note
 * If cache file of "Desktop Entry Specification" exists it will be used
 * without checking of it's validity.
 * @n
 * This is on shoulders of user of the library when checking and
 * rebuilding/reloading of the cache should take place.
 */
void xdg_init();

/**
 * Shutdown the library.
 */
void xdg_shutdown();


#ifdef __cplusplus
}
#endif
#endif /* XDG_H_ */
