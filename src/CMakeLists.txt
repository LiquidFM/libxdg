# CMake tools
include ("collect_sources")


collect_sources (src_HEADERS src_SOURCES ${CMAKE_SOURCE_DIR})


# Headers
set (exdgmime_HEADERS ${exdgmime_HEADERS} ${src_HEADERS} PARENT_SCOPE)


# Sources
set (exdgmime_SOURCES ${exdgmime_SOURCES} ${src_SOURCES} PARENT_SCOPE)
