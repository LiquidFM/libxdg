collect_sources (src_HEADERS src_SOURCES ${SOURCES_PATH})


# Headers
set (xdg_HEADERS ${xdg_HEADERS} ${src_HEADERS} PARENT_SCOPE)


# Sources
set (xdg_SOURCES ${xdg_SOURCES} ${src_SOURCES} PARENT_SCOPE)
