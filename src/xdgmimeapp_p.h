/* xdgmimeapp_p.h: Private file.  Datastructure for storing
 * the .desktop and .list files.
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

#ifndef __XDG_MIME_APP_P_H_
#define __XDG_MIME_APP_P_H_

#include "xdgmimeapp.h"


typedef struct XdgApplications XdgApplications;


XdgApplications *_xdg_mime_applications_new(void);
void _xdg_mime_applications_read_from_directory(XdgApplications *applications, const char *directory_name);
void _xdg_mime_themes_read_from_directory(XdgApplications *applications);
void _xdg_mime_applications_free(XdgApplications *applications);

const XdgArray *_xdg_mime_default_apps_lookup(XdgApplications *applications, const char *mimeType);
const XdgArray *_xdg_mime_user_apps_lookup(XdgApplications *applications, const char *mimeType);
const XdgArray *_xdg_mime_known_apps_lookup(XdgApplications *applications, const char *mimeType);
const char *_xdg_mime_app_icon_lookup(XdgApplications *applications, const XdgApp *app, const char *themeName, int size);

#endif /* __XDG_MIME_APP_P_H_ */
