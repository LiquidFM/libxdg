# CMake tools
include ("collect_sources")


collect_sources (src_HEADERS src_SOURCES ${CMAKE_SOURCE_DIR})


# Headers
set (applications_cache_HEADERS ${applications_cache_HEADERS} ${src_HEADERS} PARENT_SCOPE)


# Sources
set (applications_cache_SOURCES ${applications_cache_SOURCES} ${src_SOURCES} PARENT_SCOPE)
