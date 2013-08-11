# CMake tools
include (collect_sources)


# Sources
set (${PROJECT_NAME}_SOURCES ${PROJECT_SOURCE_DIR}/txmlparser/txml_parser.c)
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


# Include files
if (BUILD_THEMES_SPEC OR (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE))
    generate_include_file (xdg "xdg/containers/avltree.h" "src/containers/avltree.h")
    generate_include_file (xdg "xdg/containers/xdglist.h" "src/containers/xdglist.h")

    # Install include files
    install (
        FILES 
            ${CMAKE_CURRENT_SOURCE_DIR}/containers/avltree.h
            ${CMAKE_CURRENT_SOURCE_DIR}/containers/xdglist.h
            
        DESTINATION
            include/xdg/containers
    )
endif ()

if (BUILD_MIME_SPEC)
    generate_include_file (xdg "xdg/mime/xdgmime.h" "src/mime/xdgmime.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/mime/xdgmime.h DESTINATION include/xdg/mime)
endif ()

if (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE)
    generate_include_file (xdg "xdg/desktop/xdgapp.h" "src/desktop/xdgapp.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/desktop/xdgapp.h DESTINATION include/xdg/desktop)
endif ()

if (BUILD_THEMES_SPEC)
    generate_include_file (xdg "xdg/themes/xdgtheme.h" "src/themes/xdgtheme.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/themes/xdgtheme.h DESTINATION include/xdg/themes)
endif ()

if (BUILD_MENU_SPEC)
    generate_include_file (xdg "xdg/menu/xdgmenu.h" "src/menu/xdgmenu.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/menu/xdgmenu.h DESTINATION include/xdg/menu)
endif ()

generate_include_file  (xdg "xdg/xdg.h"    "src/xdg.h")
configure_include_file (xdg "xdg/config.h" "src/config/config.h.in")


# Install
install (
    TARGETS xdg
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/xdg.h DESTINATION include/xdg)


# Target - update-applications-cache
if (BUILD_UPDATE_APPLICATIONS_CACHE)
    add_subdirectory (update-applications-cache)
endif ()
