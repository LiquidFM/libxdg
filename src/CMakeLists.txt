# CMake tools
include (collect_sources)


# Sources
set (${PROJECT_NAME}_SOURCES ${CMAKE_SOURCE_DIR}/txmlparser/txml_parser.c)
set (SOURCES_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_THEMES_SPEC OR (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE))
    add_subdirectory (containers)
endif ()

add_subdirectory (basedirectory)

if (BUILD_MIME_SPEC)
    add_subdirectory (mime)
endif ()

if (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE)
    add_subdirectory (desktop)
endif ()

if (BUILD_THEMES_SPEC)
    add_subdirectory (themes)
endif ()

if (BUILD_MENU_SPEC)
    add_subdirectory (menu)
endif ()


# Target - xdg
add_library(xdg STATIC xdg.c ${${PROJECT_NAME}_SOURCES})
target_link_libraries (xdg ${${PROJECT_NAME}_LIBS})


# Include files
if (BUILD_THEMES_SPEC OR (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE))
    generate_include_file (xdg "include/xdg/containers/avltree.h" "src/containers/avltree.h")
    generate_include_file (xdg "include/xdg/containers/xdglist.h" "src/containers/xdglist.h")

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
    generate_include_file (xdg "include/xdg/mime/xdgmime.h" "src/mime/xdgmime.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/mime/xdgmime.h DESTINATION include/xdg/mime)
endif ()

if (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE)
    generate_include_file (xdg "include/xdg/desktop/xdgapp.h" "src/desktop/xdgapp.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/desktop/xdgapp.h DESTINATION include/xdg/desktop)
endif ()

if (BUILD_THEMES_SPEC)
    generate_include_file (xdg "include/xdg/themes/xdgtheme.h" "src/themes/xdgtheme.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/themes/xdgtheme.h DESTINATION include/xdg/themes)
endif ()

if (BUILD_MENU_SPEC)
    generate_include_file (xdg "include/xdg/menu/xdgmenu.h" "src/menu/xdgmenu.h")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/menu/xdgmenu.h DESTINATION include/xdg/menu)
endif ()


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
