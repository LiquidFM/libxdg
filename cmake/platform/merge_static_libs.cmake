# Copyright (C) 2012-2013  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file LICENSE for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
#
# This macro creates a true static library bundle with debug and release configurations
# TARGET - the output library, or target, that you wish to contain all of the object files
# CONFIGURATION - DEBUG, RELEASE or ALL
# LIBRARIES - a list of all of the static libraries you want merged into the TARGET
#
# Example use:
#   merge_static_libs (mytarget ALL "${MY_STATIC_LIBRARIES}")
#
# NOTE: When you call this script, make sure you quote the argument to LIBRARIES if it is a list!

function (__process_library TARGET OUTPUT)
    set (LIBRARIES ${${OUTPUT}})
    get_property (LIBS TARGET ${TARGET} PROPERTY LINK_LIBRARIES)

    foreach (LIB IN LISTS LIBS)
        if (TARGET ${LIB})
            get_property (LIB_TYPE TARGET ${LIB} PROPERTY TYPE)

            if ("${LIB_TYPE}" STREQUAL "STATIC_LIBRARY")
                get_property (LIB_LOCATION TARGET ${LIB} PROPERTY LOCATION)
                list (APPEND LIBRARIES ${LIB_LOCATION})
                __process_library (${LIB} LIBRARIES)
            else ()
                message (FATAL_ERROR "Function \"merge_static_libs\" supports only static libraries as \"TARGET\" argument")
            endif ()
        elseif (EXISTS ${LIB})
            list (APPEND LIBRARIES ${LIB})
        endif ()
    endforeach ()

    set (${OUTPUT} ${LIBRARIES} PARENT_SCOPE)
endfunction ()

function (merge_static_libs TARGET)
    set (LIBRARIES)
    get_property (LIB_TYPE TARGET ${TARGET} PROPERTY TYPE)

    if (NOT "${LIB_TYPE}" STREQUAL "STATIC_LIBRARY")
        message (FATAL_ERROR "Function \"merge_static_libs\" supports only static libraries as \"TARGET\" argument")
    endif ()

    __process_library (${TARGET} LIBRARIES)

    if (MSVC)
        # On Windows you must add aditional formatting to the LIBRARIES variable as a single string
        # for the windows libtool with each library path wrapped in "" in case it contains spaces
        string (REPLACE ";" "\" \"" LIBS "${LIBRARIES}")
        set (LIBS \"${LIBS}\")
        set_property (TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS "${LIBS}")
    elseif (APPLE AND ${CMAKE_GENERATOR} STREQUAL "Xcode")
        # iOS and OSX platforms with Xcode need slighly less formatting
        string (REPLACE ";" " " LIBS "${LIBRARIES}")
        set_property (TARGET ${TARGET} APPEND PROPERTY STATIC_LIBRARY_FLAGS "${LIBS}")
    elseif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC OR CMAKE_COMPILER_IS_GNUCC)
        # Posix platforms, including Android, require manual merging of static libraries via special script
        get_property (TARGETLOC TARGET ${TARGET} PROPERTY LOCATION)

        find_file (CONF_IN "merge_static_libs.cmake.in"
                   PATHS
                     ${CMAKE_SOURCE_DIR}/platform/cmake/platform
                     ${CMAKE_ROOT}/Modules/platform
                   NO_DEFAULT_PATH
                   NO_CMAKE_ENVIRONMENT_PATH
                   NO_CMAKE_PATH
                   NO_SYSTEM_ENVIRONMENT_PATH
                   NO_CMAKE_SYSTEM_PATH
                   NO_CMAKE_FIND_ROOT_PATH)
        if (${CONF_IN} STREQUAL "CONF_IN-NOTFOUND")
            message (FATAL_ERROR "File \"merge_static_libs.cmake.in\" not found!")
        endif ()

        configure_file (${CONF_IN} ${CMAKE_CURRENT_BINARY_DIR}/merge_static_libs-${TARGET}.cmake @ONLY)
        add_custom_command (TARGET
                            ${TARGET}
                            POST_BUILD COMMAND
                            ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/merge_static_libs-${TARGET}.cmake
                            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/merge_static_libs-${TARGET}.cmake)
    else ()
        message (WARNING "The \"merge_static_libs\" script does not support this platform")
    endif ()
endfunction ()
