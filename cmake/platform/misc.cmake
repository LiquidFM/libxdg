# Copyright (C) 2012-2015  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.


# Create/Remove temporary files
function (begin_tmp_file FILE)
    set (_counter 0)
    if (NOT WINDOWS)
        set (_base "/tmp/.cmake-tmp")
    else ()
        set (_base ".cmake-tmp")
    endif ()
    while (EXISTS "${_base}${_counter}")
        math (EXPR _counter "${_counter} + 1")
    endwhile ()

    set (${FILE} "${_base}${_counter}" PARENT_SCOPE)
endfunction ()

function (end_tmp_file FILE)
    file (REMOVE "${${FILE}}")
    set (${FILE} "" PARENT_SCOPE)
endfunction ()


# Evaluate expression
# Suggestion from the Wiki: http://cmake.org/Wiki/CMake/Language_Syntax
# Unfortunately, no built-in stuff for this: http://public.kitware.com/Bug/view.php?id=4034
macro (eval expr)
    begin_tmp_file (TMP_FILE)
    file (WRITE "${TMP_FILE}" "${expr}")
    include ("${TMP_FILE}")
    end_tmp_file (TMP_FILE)
endmacro ()


# Search for subdirectories in CURDIR
function (subdir_list RESULT CURDIR)
    file (GLOB CHILDREN RELATIVE ${CURDIR} ${CURDIR}/*)

    foreach (child ${CHILDREN})
        if (IS_DIRECTORY ${CURDIR}/${child})
            list (APPEND DIRLIST ${child})
        endif ()
    endforeach ()

    set (${RESULT} ${DIRLIST} PARENT_SCOPE)
endfunction ()


# Discover GCC predefined macros
function (gcc_predefined_macros RESULT)
    begin_tmp_file (TMP_FILE)
    file (WRITE "${TMP_FILE}" "int main(void) { return 0; }")
    string(REGEX REPLACE " +" ";" FLAGS "${CMAKE_C_FLAGS}")
    execute_process (COMMAND ${CMAKE_C_COMPILER} ${FLAGS} -xc -E -P -v -dD ${TMP_FILE} RESULT_VARIABLE RES OUTPUT_VARIABLE RES_STRING ERROR_QUIET)
    end_tmp_file (TMP_FILE)
    
    if (NOT RES EQUAL 0)
        message (FATAL_ERROR "Failed to get predefined macros!")
    else ()
        string(REGEX REPLACE "\n" ";" RES_STRING "${RES_STRING}")
        string(REGEX REPLACE "#define[ \t]+([^ \t]+)[ \t]+([^ \t;]+)" "\\1=\\2" RES_STRING "${RES_STRING}")
        string(REGEX REPLACE ";int main.*$" "" RES_STRING "${RES_STRING}")
        set (${RESULT} ${RES_STRING} PARENT_SCOPE)
    endif ()
endfunction ()


# Determine GNU target triplet
function (__get_compiler_target)
    set (LANG $ENV{LANG})
    set ($ENV{LANG} "C")

    if (CMAKE_C_COMPILER)
        execute_process (COMMAND ${CMAKE_C_COMPILER} -v -c RESULT_VARIABLE RES ERROR_VARIABLE RES_STRING)
    elseif (CMAKE_CXX_COMPILER)
        execute_process (COMMAND ${CMAKE_CXX_COMPILER} -v -c RESULT_VARIABLE RES ERROR_VARIABLE RES_STRING)
    endif ()

    set ($ENV{LANG} ${LANG})

    if (RES EQUAL 0)
        string (REGEX REPLACE "^(.*)Target: ([^\n]+)(.*)$" "\\2" _TARGET_ "${RES_STRING}")
    else ()
        message (FATAL_ERROR "Unable to determine TARGET system!")
    endif ()

    set (TARGET ${_TARGET_} PARENT_SCOPE)
endfunction ()


# Determine SYSROOT directory
function (__get_sysroot_directory)
    set (_SYSROOT)
    list (APPEND _SYSROOT ${CMAKE_INCLUDE_PATH})
    list (APPEND _SYSROOT ${CMAKE_SYSTEM_INCLUDE_PATH})
    list (APPEND _SYSROOT ${CMAKE_PREFIX_PATH})
    list (APPEND _SYSROOT ${CMAKE_SYSTEM_PREFIX_PATH})
    list (APPEND _SYSROOT ${CMAKE_FIND_ROOT_PATH})
    list (REMOVE_DUPLICATES _SYSROOT)
    list (REVERSE _SYSROOT)
    list (GET _SYSROOT 0 _SYSROOT)
    set (SYSROOT ${_SYSROOT} PARENT_SCOPE)
endfunction ()
