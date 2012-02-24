# Target - update-applications-cache
add_executable (update-applications-cache main.c)
target_link_libraries (update-applications-cache ${xdg_LIBS} xdg)

# Install
install (
    TARGETS update-applications-cache
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
