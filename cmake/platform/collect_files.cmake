# Copyright (C) 2012-2015  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file LICENSE for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


function (collect_files THIS_FILES FILES_MASK RELATIVE_DIR)

    if (NOT RELATIVE_DIR)
        set (RELATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    file (GLOB_RECURSE files_0 RELATIVE "${RELATIVE_DIR}" FOLLOW_SYMLINKS "${FILES_MASK}")
    list (APPEND files_0 ${${THIS_FILES}})
    set (${THIS_FILES} ${files_0} PARENT_SCOPE)

endfunction ()


function (collect_files_in WHERE THIS_FILES FILES_MASK RELATIVE_DIR)

    if (NOT RELATIVE_DIR)
        set (RELATIVE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    file (GLOB_RECURSE files_0 RELATIVE "${RELATIVE_DIR}" FOLLOW_SYMLINKS "${WHERE}/${FILES_MASK}")
    list (APPEND files_0 ${${THIS_FILES}})
    set (${THIS_FILES} ${files_0} PARENT_SCOPE)

endfunction ()
