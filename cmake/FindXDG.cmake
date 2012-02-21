# - Find xdg
# Find the xdg library
#
#  This module defines the following variables:
#     XDG_FOUND   - true if the library was found.
#     XDG_INCLUDE - where to find xdg.h.
#     XDG_LIBRARY - libxdg.
#
# Copyright (C) 2012  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.

find_path (XDG_INCLUDE
          NAMES "xdg.h"
          PATH_SUFFIXES "xdg"
          DOC "The XDG include directory"
)
string (REGEX REPLACE "/xdg$" "" XDG_INCLUDE ${XDG_INCLUDE})

find_library(XDG_LIBRARY NAMES xdg)


# handle the QUIETLY and REQUIRED arguments and set XDG_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(XDG DEFAULT_MSG XDG_INCLUDE XDG_LIBRARY)

mark_as_advanced(XDG_INCLUDE XDG_LIBRARY)
