project (xdg)

# Project header
project_static_library_header_default ()

# Includes
include_directories (${CMAKE_SOURCE_DIR}/txmlparser)

# Targets
add_subdirectory (src)

# Version
build_version_file (xdg 0.2.1)

# Documentation
add_documentation ("XDG" 0.2.1 "This library is implementation of several freedesktop.org specifications.")
