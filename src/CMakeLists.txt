project (xdg)

# Project header
project_static_library_header_default ()

# CMake tools
include (platform/collect_sources)

# Includes
include_directories (${CMAKE_SOURCE_DIR}/txmlparser)

# Sources
set (${PROJECT_NAME}_SOURCES ${CMAKE_SOURCE_DIR}/txmlparser/txml_parser.c)
set (SOURCES_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_THEMES_SPEC OR (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE))
    add_subdirectory (containers)
endif ()

add_subdirectory (basedirectory)

if (BUILD_MIME_SPEC)
    add_subdirectory (mime)
    set (CONFIG_MIME_SPEC "#define MIME_SPEC")
else ()
    set (CONFIG_MIME_SPEC "// #define MIME_SPEC")
endif ()

if (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE)
    add_subdirectory (desktop)
    set (CONFIG_DESKTOP_SPEC "#define DESKTOP_SPEC")
else ()
    set (CONFIG_DESKTOP_SPEC "// #define DESKTOP_SPEC")
endif ()

if (BUILD_THEMES_SPEC)
    add_subdirectory (themes)
    set (CONFIG_THEMES_SPEC "#define THEMES_SPEC")
else ()
    set (CONFIG_THEMES_SPEC "// #define THEMES_SPEC")
endif ()

if (BUILD_MENU_SPEC)
    add_subdirectory (menu)
    if (NOT BUILD_DESKTOP_SPEC)
        message (FATAL_ERROR "Implementation of \"Desktop Menu Specification\" depends on implementation of \"Desktop Entry Specification\"! You have to set BUILD_DESKTOP_SPEC to ON if you need \"Desktop Menu Specification\".")
    endif ()
    set (CONFIG_MENU_SPEC "#define MENU_SPEC")
else ()
    set (CONFIG_MENU_SPEC "// #define MENU_SPEC")
endif ()

# Target - xdg
add_library(xdg STATIC xdg.c ${${PROJECT_NAME}_SOURCES})
target_link_libraries (xdg ${${PROJECT_NAME}_LIBS})

# Documentation
add_documentation ("XDG" 0.2.1 "This library is implementation of several freedesktop.org specifications.")

# Include files
set (HEADERS_TO_BE_INSTALLED)

if (BUILD_THEMES_SPEC OR (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE))
    list (APPEND HEADERS_TO_BE_INSTALLED "containers/avltree.h:containers/")
    list (APPEND HEADERS_TO_BE_INSTALLED "containers/xdglist.h:containers/")
endif ()

if (BUILD_MIME_SPEC)
    list (APPEND HEADERS_TO_BE_INSTALLED "mime/xdgmime.h:mime/")
endif ()

if (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE)
    list (APPEND HEADERS_TO_BE_INSTALLED "desktop/xdgapp.h:desktop/")
endif ()

if (BUILD_THEMES_SPEC)
    list (APPEND HEADERS_TO_BE_INSTALLED "themes/xdgtheme.h:themes/")
endif ()

if (BUILD_MENU_SPEC)
    list (APPEND HEADERS_TO_BE_INSTALLED "menu/xdgmenu.h:menu/")
endif ()

list (APPEND HEADERS_TO_BE_INSTALLED "xdg.h")

configure_file ("config/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
list (APPEND HEADERS_TO_BE_INSTALLED "${CMAKE_CURRENT_BINARY_DIR}/config.h")


# Install
install_header_files (xdg ${HEADERS_TO_BE_INSTALLED})


# Target - update-applications-cache
if (BUILD_UPDATE_APPLICATIONS_CACHE)
    add_subdirectory (update-applications-cache)
endif ()
