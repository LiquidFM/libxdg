project (xdg)

# Project header
project_cxx_library_header_default ()

# Includes
include_directories (${CMAKE_SOURCE_DIR}/txmlparser)

# Targets
add_subdirectory (src)

if (BUILD_MIME_SPEC)
    set (CONFIG_MIME_SPEC "#define MIME_SPEC")
else ()
    set (CONFIG_MIME_SPEC "// #define MIME_SPEC")
endif ()

if (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE)
    set (CONFIG_DESKTOP_SPEC "#define DESKTOP_SPEC")
else ()
    set (CONFIG_DESKTOP_SPEC "// #define DESKTOP_SPEC")
endif ()

if (BUILD_THEMES_SPEC)
    set (CONFIG_THEMES_SPEC "#define THEMES_SPEC")
else ()
    set (CONFIG_THEMES_SPEC "// #define THEMES_SPEC")
endif ()

if (BUILD_MENU_SPEC)
    if (NOT BUILD_DESKTOP_SPEC)
        message (FATAL_ERROR "Implementation of \"Desktop Menu Specification\" depends on implementation of \"Desktop Entry Specification\"! You have to set BUILD_DESKTOP_SPEC to ON if you need \"Desktop Menu Specification\".")
    endif ()
    set (CONFIG_MENU_SPEC "#define MENU_SPEC")
else ()
    set (CONFIG_MENU_SPEC "// #define MENU_SPEC")
endif ()

# Version
build_version_file (xdg 0.2.1)

# Documentation
add_documentation ("XDG" 0.2.1 "This library is implementation of several freedesktop.org specifications.")
