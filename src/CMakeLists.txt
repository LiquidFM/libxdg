# CMake tools
include ("collect_sources")


# Sources
set (SOURCES_PATH ${CMAKE_CURRENT_SOURCE_DIR})
 
add_subdirectory ("containers")
add_subdirectory ("basedirectory")


if (BUILD_MIME_SPEC)
    add_subdirectory ("mime")
    
    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/mime/xdgmime.h DESTINATION include/xdg/mime)
endif ()

if (BUILD_DESKTOP_SPEC OR BUILD_UPDATE_APPLICATIONS_CACHE)
    add_subdirectory ("desktop")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/desktop/xdgapp.h DESTINATION include/xdg/desktop)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/containers/xdglist.h DESTINATION include/xdg/containers)
endif ()

if (BUILD_THEMES_SPEC)
    add_subdirectory ("themes")

    # Install include files
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/themes/xdgtheme.h DESTINATION include/xdg/themes)
endif ()


# Target xdg
add_library(xdg STATIC xdg.c ${xdg_SOURCES})
target_link_libraries (xdg ${xdg_LIBS})

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
    add_subdirectory ("update-applications-cache")
endif ()
