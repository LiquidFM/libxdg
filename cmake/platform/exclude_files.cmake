# Copyright (C) 2012  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying _file LICENSE for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


function (exclude_files MASK THIS_FILES)
    # "THIS_FILES" contains name of the actual variable passed into here.
    set (_FILES)

    foreach (_file ${${THIS_FILES}})
        if (NOT ${_file} MATCHES ${MASK})
            list (APPEND _FILES ${_file})
        endif ()
    endforeach ()
    set (${THIS_FILES} ${_FILES} PARENT_SCOPE)

endfunction (exclude_files)
