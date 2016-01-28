# Copyright (C) 2012-2015  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.


# http://www.cmake.org/Wiki/CMake_Useful_Variables
# Since CMake 2.1 the install rule depends on all,
# i.e. everything will be built before installing.
# If you don't like this, set this one to true.
set (CMAKE_SKIP_INSTALL_ALL_DEPENDENCY YES)


include_directories (${CMAKE_BINARY_DIR}/_headers_)


function (__setup_global_headers)
    set (_GLOBAL_HEADERS ${CMAKE_BINARY_DIR}/_headers_)

    string (TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
    set (_PROJECT_GLOBAL_HEADERS ${_GLOBAL_HEADERS}/${PROJECT_NAME_LOWER})

    file (MAKE_DIRECTORY ${_PROJECT_GLOBAL_HEADERS})

    set (GLOBAL_HEADERS ${_GLOBAL_HEADERS} PARENT_SCOPE)
    set (PROJECT_GLOBAL_HEADERS ${_PROJECT_GLOBAL_HEADERS} PARENT_SCOPE)
endfunction ()


macro (__process_files_to_install MACROS EXT PREFIX ...)
    set (ARGS ${ARGV})
    if ("${PREFIX}" STREQUAL "")
        list (REMOVE_AT ARGS 0 1)
    else ()
        list (REMOVE_AT ARGS 0 1 2)
        set (PREFIX_LOC "${PREFIX}/")
    endif ()

    foreach (ARG IN LISTS ARGS)
        if (${ARG} MATCHES "^.+:[^:]+$")
            string (REGEX REPLACE "^(.+):[^:]+$"       "\\1" HEADER_SRC     "${ARG}")
            string (REGEX REPLACE "^.+:([^:]+)$"       "\\1" HEADER_DST     "${ARG}")
            string (REGEX REPLACE "^((.+[\\/])*).+$"   "\\1" HEADER_DST_DIR "${PREFIX_LOC}${HEADER_DST}")

            if (NOT ${ARG} MATCHES "^[\\/]")
                set (HEADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_SRC}")
            endif ()
        else ()
            if (${ARG} MATCHES "^[\\/]")
                set (HEADER_SRC "${ARG}")
            else ()
                set (HEADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${ARG}")
            endif ()

            if (IS_DIRECTORY ${HEADER_SRC})
                set (HEADER_DST)
                set (HEADER_DST_DIR "${PREFIX_LOC}")
            else ()
                string (REGEX REPLACE "^.*[\\/]([^\\/]+)$" "\\1" HEADER_DST     "${ARG}")
                string (REGEX REPLACE "^((.+[\\/])*).+$"   "\\1" HEADER_DST_DIR "${PREFIX_LOC}${HEADER_DST}")
            endif ()
        endif ()

        if (NOT EXISTS "${HEADER_SRC}")
            message (FATAL_ERROR "Invalid argument: ${ARG}\nSource path '${HEADER_SRC}' does not exist!")
        endif ()

        if (IS_DIRECTORY "${HEADER_SRC}")
            if (NOT "${HEADER_DST}" STREQUAL "" AND NOT "${HEADER_DST}" MATCHES "[\\/]$")
                message (FATAL_ERROR "Invalid argument: ${ARG}\nDestination path '${HEADER_DST}' is invalid (have you forgotten suffix '/'?)")
            endif ()

            string (REGEX MATCHALL "[^,]+" EXTENSIONS "${EXT}")
            foreach (extension IN LISTS EXTENSIONS)
                file (GLOB CHILDREN RELATIVE ${HEADER_SRC} ${HEADER_SRC}/${extension})

                foreach (child ${CHILDREN})
                    if (NOT IS_DIRECTORY "${HEADER_SRC}/${child}")
                        eval ("${MACROS} (\"${HEADER_SRC}/${child}\" \"${PREFIX_LOC}${HEADER_DST}\" \"${PREFIX_LOC}${HEADER_DST}${child}\")")
                    endif ()
                endforeach ()
            endforeach ()
        else ()
            if (${HEADER_DST} MATCHES "[\\/]$")
                set (HEADER_DST_DIR "${PREFIX_LOC}${HEADER_DST}")
                string (REGEX REPLACE "^.*[\\/]([^\\/]+)$" "${HEADER_DST}\\1" HEADER_DST "${HEADER_SRC}")
            endif ()

            eval ("${MACROS} (\"${HEADER_SRC}\" \"${HEADER_DST_DIR}\" \"${PREFIX_LOC}${HEADER_DST}\")")
        endif ()
    endforeach ()
endmacro ()


macro (__install_header_files_with_prefix_macro A1 A2 A3)
    add_custom_command (OUTPUT ${GLOBAL_HEADERS}/${A3}
                        COMMAND ${CMAKE_COMMAND} -E copy ${A1} ${GLOBAL_HEADERS}/${A3}
                        DEPENDS ${A1})

    list (APPEND GENERATED_FILES "${GLOBAL_HEADERS}/${A3}")

    if (CMAKE_CROSSCOMPILING)
        install (FILES ${GLOBAL_HEADERS}/${A3}
                 DESTINATION ${TARGET}/include/${A2}
                 COMPONENT headers)
    else ()
        install (FILES ${GLOBAL_HEADERS}/${A3}
                 DESTINATION include/${A2}
                 COMPONENT headers)
    endif ()
endmacro ()


function (install_header_files_with_prefix NAME PREFIX ...)
    if (NOT TARGET ${NAME}_headers)
        __setup_global_headers ()
        __get_compiler_target ()
        set (GENERATED_FILES)

        set (ARGS ${ARGV})
        if ("${PREFIX}" STREQUAL "")
            list (REMOVE_AT ARGS 0)
        else ()
            list (REMOVE_AT ARGS 0 1)
        endif ()

        __process_files_to_install (__install_header_files_with_prefix_macro "*.h,*.hpp,*.hxx" "${PREFIX}" ${ARGS})

        add_custom_target (${NAME}_headers DEPENDS ${GENERATED_FILES})
        add_dependencies (${NAME} ${NAME}_headers)
    else ()
        message (FATAL_ERROR "Function 'install_header_files_with_prefix' can be called only once in each directory!")
    endif ()
endfunction ()


function (install_header_files NAME ...)
    if (NOT TARGET ${NAME}_headers)
        set (ARGS ${ARGV})
        list (REMOVE_AT ARGS 0)
        string (TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
        install_header_files_with_prefix (${NAME} ${PROJECT_NAME_LOWER} ${ARGS})
    else ()
        message (FATAL_ERROR "Function 'install_header_files' can be called only once in each directory!")
    endif ()
endfunction ()


macro (__install_build_time_header_files A1 A2 A3)
    add_custom_command (OUTPUT ${GLOBAL_HEADERS}/${A3}
                        COMMAND ${CMAKE_COMMAND} -E copy ${A1} ${GLOBAL_HEADERS}/${A3}
                        DEPENDS ${A1})

    list (APPEND GENERATED_FILES "${GLOBAL_HEADERS}/${A3}")
endmacro ()

function (install_build_time_header_files NAME ...)
    if (NOT TARGET ${NAME}_build_time_headers)
        __setup_global_headers ()

        set (GENERATED_FILES)
        set (ARGS ${ARGV})
        list (REMOVE_AT ARGS 0)

        __process_files_to_install (__install_build_time_header_files "*.h,*.hpp,*.hxx" "" ${ARGS})

        add_custom_target (${NAME}_build_time_headers DEPENDS ${GENERATED_FILES})
        add_dependencies (${NAME} ${NAME}_build_time_headers)
    else ()
        message (FATAL_ERROR "Function 'install_build_time_header_files' can be called only once in each directory!")
    endif ()
endfunction ()


macro (__install_cmake_files_macro A1 A2 A3)
    install (FILES ${A1} DESTINATION share/cmake/Modules/${A2} COMPONENT cmake_scripts)
endmacro ()

function (install_cmake_files ...)
    __process_files_to_install (__install_cmake_files_macro "*.cmake" "" ${ARGV})
endfunction ()


macro (__install_files_macro A1 A2 A3)
    install (FILES ${A1} DESTINATION ${A2})
endmacro ()

function (install_files DESTINATION ...)
    set (ARGS ${ARGV})
    if (NOT "${DESTINATION}" STREQUAL "")
        list (REMOVE_AT ARGS 0)
    endif ()
    __process_files_to_install (__install_files_macro "*" "${DESTINATION}" ${ARGS})
endfunction ()


macro (__install_programs_macro A1 A2 A3)
    install (PROGRAMS ${A1} DESTINATION ${A2})
endmacro ()

function (install_programs DESTINATION ...)
    set (ARGS ${ARGV})
    if (NOT "${DESTINATION}" STREQUAL "")
        list (REMOVE_AT ARGS 0)
    endif ()
    __process_files_to_install (__install_programs_macro "*" "${DESTINATION}" ${ARGS})
endfunction ()


function (install_target NAME)
    get_target_property (TARGET_TYPE ${NAME} TYPE)

    if ("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
        set (INSTALL_COMPONENT runtime)
    else ()
        set (INSTALL_COMPONENT libraries)
    endif ()

    if (CMAKE_CROSSCOMPILING)
        __get_compiler_target ()
        install (TARGETS ${NAME}
                 RUNTIME DESTINATION ${TARGET}/bin
                 LIBRARY DESTINATION ${TARGET}/lib
                 ARCHIVE DESTINATION ${TARGET}/lib
                 COMPONENT ${INSTALL_COMPONENT})
    else ()
        install (TARGETS ${NAME}
                 RUNTIME DESTINATION bin
                 LIBRARY DESTINATION lib
                 ARCHIVE DESTINATION lib
                 COMPONENT ${INSTALL_COMPONENT})
    endif ()
endfunction ()
