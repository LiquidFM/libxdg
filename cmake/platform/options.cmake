# Copyright (C) 2012-2013  Dmitriy Vilkov <dav.daemon@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.


set (_PLATFORM_AVAILABLE_OPTIONS "")

macro (platform_option_define _name _description _initialvalue)
    set (_PLATFORM_AVAILABLE_OPTIONS_INITALVALUE_${_name} ${_initialvalue})
    set (_PLATFORM_AVAILABLE_OPTIONS_DESCRIPTION_${_name} ${_description})
    list (APPEND _PLATFORM_AVAILABLE_OPTIONS ${_name})
endmacro ()

macro (platform_option_value _name _value)
    set (_PLATFORM_AVAILABLE_OPTIONS_INITALVALUE_${_name} ${_value})
endmacro ()

macro (platform_options_begin ...)
    foreach (_file_name ${ARGV})
        file (STRINGS ${_file_name} _OPTIONS_LIST)

        foreach (_option ${_OPTIONS_LIST})
            string (REGEX MATCH "^([A-Za-z]+[^ \t]*)[ \t]+\"(.*)\"[ \t]+([01])$" _regex_matched ${_option})

            if (_regex_matched)
                PLATFORM_OPTION_DEFINE (${CMAKE_MATCH_1} ${CMAKE_MATCH_2} ${CMAKE_MATCH_3})
            else ()
                string (REGEX MATCH "^#" _regex_matched ${_option})

                if (NOT _regex_matched)
                    message (FATAL_ERROR "Invalid declaration of platform option \"${_option}\".")
                endif ()
            endif ()

        endforeach ()
    endforeach ()
endmacro ()

macro (platform_options_end)
    list (REMOVE_DUPLICATES _PLATFORM_AVAILABLE_OPTIONS)

    foreach (_name ${_PLATFORM_AVAILABLE_OPTIONS})
        option (${_name} "${_PLATFORM_AVAILABLE_OPTIONS_DESCRIPTION_${_name}}" ${_PLATFORM_AVAILABLE_OPTIONS_INITALVALUE_${_name}})

        if (${_PLATFORM_AVAILABLE_OPTIONS_INITALVALUE_${_name}})
            add_definitions (-DPLATFORM_${_name}=1)
        else ()
            add_definitions (-DPLATFORM_${_name}=0)
        endif ()
    endforeach ()

    message (STATUS "Features:")

    set (_MAX_FEATURE_LENGTH 0)
    foreach (_name ${_PLATFORM_AVAILABLE_OPTIONS})
        string (LENGTH ${_name} _NAME_LENGTH)
        if (_NAME_LENGTH GREATER _MAX_FEATURE_LENGTH)
            set (_MAX_FEATURE_LENGTH ${_NAME_LENGTH})
        endif ()
    endforeach ()

    foreach (_name ${_PLATFORM_AVAILABLE_OPTIONS})
        string (LENGTH ${_name} _NAME_LENGTH)

        set (_MESSAGE " ${_name} ")

        foreach (IGNORE RANGE ${_NAME_LENGTH} ${_MAX_FEATURE_LENGTH})
            set (_MESSAGE "${_MESSAGE}.")
        endforeach ()

        if (${_name})
            set (_MESSAGE "${_MESSAGE} ON")
        else ()
            set (_MESSAGE "${_MESSAGE} OFF")
        endif ()

        message (STATUS "${_MESSAGE}")
    endforeach ()
endmacro ()
