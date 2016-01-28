# Copyright (C) 2012-2015  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file LICENSE for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


# Misc macros and functions
include (platform/misc)

# Sets up Compiler and Linker flags in temporary variables.
include (platform/compiler_flags)

# Sets up BUILD_TYPE variables.
include (platform/project_build_type)


macro (initialize_platform)
    if (WIN32)
        # Fix path delimeters
        file (TO_CMAKE_PATH ${CMAKE_INSTALL_PREFIX} CMAKE_INSTALL_PREFIX)
    endif ()

    setup_compiler ()

    message (STATUS "CMake generates " ${CMAKE_GENERATOR})
endmacro ()


function (set_target_compiler_flags NAME ...)
    set (ARGS ${ARGV})
    list (REMOVE_AT ARGS 0)
    get_target_property (LANG ${NAME} LINKER_LANGUAGE)

    process_compiler_features (NO ${LANG} ${ARGS})

    string (REGEX REPLACE ";" " " COMPILER_${LANG}_FLAGS "${COMPILER_${LANG}_FLAGS}")
    set_target_properties (${NAME} PROPERTIES COMPILE_FLAGS "${COMPILER_${LANG}_FLAGS}")

    string (REGEX REPLACE ";" " " LINKER_${LANG}_FLAGS "${LINKER_${LANG}_FLAGS}")
    set_target_properties (${NAME} PROPERTIES LINK_FLAGS "${LINKER_${LANG}_FLAGS}")
endfunction ()


function (set_target_compiler_flags_default NAME ...)
    set (ARGS ${ARGV})
    list (REMOVE_AT ARGS 0)
    get_target_property (LANG ${NAME} LINKER_LANGUAGE)

    process_compiler_features (YES ${LANG} ${ARGS})

    string (REGEX REPLACE ";" " " COMPILER_${LANG}_FLAGS "${COMPILER_${LANG}_FLAGS}")
    set_target_properties (${NAME} PROPERTIES COMPILE_FLAGS "${COMPILER_${LANG}_FLAGS}")

    string (REGEX REPLACE ";" " " LINKER_${LANG}_FLAGS "${LINKER_${LANG}_FLAGS}")
    set_target_properties (${NAME} PROPERTIES LINK_FLAGS "${LINKER_${LANG}_FLAGS}")
endfunction ()


function (project_header ...)
    get_property (LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
    process_compiler_features (NO "${LANGUAGES}" ${ARGV})

    foreach (l IN LISTS LANGUAGES)
        string (REGEX REPLACE ";" " " COMPILER_${l}_FLAGS "${COMPILER_${l}_FLAGS}")
        set (CMAKE_${l}_FLAGS "${CMAKE_${l}_FLAGS} ${COMPILER_${l}_FLAGS}" PARENT_SCOPE)

        string (REGEX REPLACE ";" " " LINKER_${l}_FLAGS "${LINKER_${l}_FLAGS}")
        set (CMAKE_${l}_LINK_FLAGS "${CMAKE_${l}_LINK_FLAGS} ${LINKER_${l}_FLAGS}" PARENT_SCOPE)
    endforeach ()
endfunction ()


function (project_header_default ...)
    get_property (LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
    process_compiler_features (YES "${LANGUAGES}" ${ARGV})

    foreach (l IN LISTS LANGUAGES)
        string (REGEX REPLACE ";" " " COMPILER_${l}_FLAGS "${COMPILER_${l}_FLAGS}")
        set (CMAKE_${l}_FLAGS "${CMAKE_${l}_FLAGS} ${COMPILER_${l}_FLAGS}" PARENT_SCOPE)

        string (REGEX REPLACE ";" " " LINKER_${l}_FLAGS "${LINKER_${l}_FLAGS}")
        set (CMAKE_${l}_LINK_FLAGS "${CMAKE_${l}_LINK_FLAGS} ${LINKER_${l}_FLAGS}" PARENT_SCOPE)
    endforeach ()
endfunction ()


macro (project_executable_header_default)
    if (NOT DEFINED ENABLE_POSITION_INDEPENDENT_CODE)
        set (ENABLE_POSITION_INDEPENDENT_CODE NO)
    endif ()
    if (NOT DEFINED ENABLE_RUNTIME_TYPE_INFORMATION)
        set (ENABLE_RUNTIME_TYPE_INFORMATION NO)
    endif ()

    project_header_default ("POSITION_INDEPENDENT_CODE:${ENABLE_POSITION_INDEPENDENT_CODE}"
                            "RTTI:${ENABLE_RUNTIME_TYPE_INFORMATION}")
endmacro ()


macro (project_static_library_header_default)
    if (NOT DEFINED ENABLE_POSITION_INDEPENDENT_CODE)
        set (ENABLE_POSITION_INDEPENDENT_CODE NO)
    endif ()
    if (NOT DEFINED ENABLE_RUNTIME_TYPE_INFORMATION)
        set (ENABLE_RUNTIME_TYPE_INFORMATION NO)
    endif ()

    project_header_default ("POSITION_INDEPENDENT_CODE:${ENABLE_POSITION_INDEPENDENT_CODE}"
                            "RTTI:${ENABLE_RUNTIME_TYPE_INFORMATION}")
endmacro ()


macro (project_shared_library_header_default)
    if (NOT DEFINED ENABLE_POSITION_INDEPENDENT_CODE)
        set (ENABLE_POSITION_INDEPENDENT_CODE NO)
    endif ()
    if (NOT DEFINED ENABLE_RUNTIME_TYPE_INFORMATION)
        set (ENABLE_RUNTIME_TYPE_INFORMATION NO)
    endif ()

    project_header_default ("SHARED_CRT:YES"
                            "POSITION_INDEPENDENT_CODE:${ENABLE_POSITION_INDEPENDENT_CODE}"
                            "RTTI:${ENABLE_RUNTIME_TYPE_INFORMATION}")
endmacro ()


macro (project_library_header_default)
    if (NOT DEFINED ENABLE_POSITION_INDEPENDENT_CODE)
        set (ENABLE_POSITION_INDEPENDENT_CODE NO)
    endif ()
    if (NOT DEFINED ENABLE_RUNTIME_TYPE_INFORMATION)
        set (ENABLE_RUNTIME_TYPE_INFORMATION NO)
    endif ()
    if (NOT DEFINED BUILD_SHARED_LIBS)
        set (BUILD_SHARED_LIBS NO)
    endif ()

    project_header_default ("SHARED_CRT:${BUILD_SHARED_LIBS}"
                            "POSITION_INDEPENDENT_CODE:${ENABLE_POSITION_INDEPENDENT_CODE}"
                            "RTTI:${ENABLE_RUNTIME_TYPE_INFORMATION}")
endmacro ()
