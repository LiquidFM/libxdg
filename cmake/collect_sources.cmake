# Copyright (C) 2012  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.

function (collect_sources THIS_HEADERS THIS_SOURCES RELATIVE_DIR)
    # "THIS_HEADERS", "THIS_SOURCES" and "RELATIVE_DIR" contains names of the actual variables passed into here.
    
    if (NOT RELATIVE_DIR)
        set (RELATIVE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    
    # Headers
    file (GLOB_RECURSE src_HEADERS_0 RELATIVE ${RELATIVE_DIR} FOLLOW_SYMLINKS "*.h")
    set (src_HEADERS ${src_HEADERS} ${src_HEADERS_0})
    
    file (GLOB_RECURSE src_HEADERS_0 RELATIVE ${RELATIVE_DIR} FOLLOW_SYMLINKS "*.hpp")
    set (src_HEADERS ${src_HEADERS} ${src_HEADERS_0})
    
    if (WIN32)
        set (tmp_src_HEADERS "")
        foreach (src_HEADER ${src_HEADERS})
            if (NOT ${src_HEADER} MATCHES ".*/unix/.*")
                set (tmp_src_HEADERS ${tmp_src_HEADERS} ${src_HEADER})
            endif ()
        endforeach ()
        set (src_HEADERS ${tmp_src_HEADERS})
    elseif (UNIX)
        set (tmp_src_HEADERS "")
        foreach (src_HEADER ${src_HEADERS})
            if (NOT ${src_HEADER} MATCHES ".*/win32/.*")
                set (tmp_src_HEADERS ${tmp_src_HEADERS} ${src_HEADER})
            endif ()
        endforeach ()
        set (src_HEADERS ${tmp_src_HEADERS})
    else ()
        message (FATAL_ERROR "Unknown operational system!")
    endif ()
    set (${THIS_HEADERS} ${${THIS_HEADERS}} ${src_HEADERS} PARENT_SCOPE)
    
    
    # Sources
    file (GLOB_RECURSE src_SOURCES_0 RELATIVE ${RELATIVE_DIR} FOLLOW_SYMLINKS "*.cpp")
    set (src_SOURCES ${src_SOURCES} ${src_SOURCES_0})

    file (GLOB_RECURSE src_SOURCES_0 RELATIVE ${RELATIVE_DIR} FOLLOW_SYMLINKS "*.cxx")
    set (src_SOURCES ${src_SOURCES} ${src_SOURCES_0})

    file (GLOB_RECURSE src_SOURCES_0 RELATIVE ${RELATIVE_DIR} FOLLOW_SYMLINKS "*.c")
    set (src_SOURCES ${src_SOURCES} ${src_SOURCES_0})

    if (WIN32)
        set (tmp_src_SOURCES "")
        foreach (src_SOURCE ${src_SOURCES})
            if (NOT ${src_SOURCE} MATCHES ".*/unix/.*")
                set (tmp_src_SOURCES ${tmp_src_SOURCES} ${src_SOURCE})
            endif ()
        endforeach ()
        set (src_SOURCES ${tmp_src_SOURCES})
    elseif (UNIX)
        set (tmp_src_SOURCES "")
        foreach (src_SOURCE ${src_SOURCES})
            if (NOT ${src_SOURCE} MATCHES ".*/win32/.*")
                set (tmp_src_SOURCES ${tmp_src_SOURCES} ${src_SOURCE})
            endif ()
        endforeach ()
        set (src_SOURCES ${tmp_src_SOURCES})
    else ()
        message (FATAL_ERROR "Unknown operational system!")
    endif ()
    set (${THIS_SOURCES} ${${THIS_SOURCES}} ${src_SOURCES} PARENT_SCOPE)
    
endfunction (collect_sources)
