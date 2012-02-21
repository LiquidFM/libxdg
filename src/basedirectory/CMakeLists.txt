collect_sources (src_HEADERS src_SOURCES ${SOURCES_PATH})


# Headers
set (exdgmime_HEADERS ${exdgmime_HEADERS} ${src_HEADERS} PARENT_SCOPE)


# Sources
set (exdgmime_SOURCES ${exdgmime_SOURCES} ${src_SOURCES} PARENT_SCOPE)
