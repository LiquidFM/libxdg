cmake_minimum_required (VERSION 2.8)
project (xdg)


# Main information
if (CMAKE_CROSSCOMPILING)
    message (STATUS "Cross compiling to: " ${CMAKE_SYSTEM_NAME} " " ${CMAKE_SYSTEM_VERSION})
else ()
    message (STATUS "System: " ${CMAKE_SYSTEM_NAME} " " ${CMAKE_SYSTEM_VERSION})
endif ()
message (STATUS "Processor: " ${CMAKE_HOST_SYSTEM_PROCESSOR})
if (MSVC)
	message(STATUS "Compiler: MSVC, version: " ${MSVC_VERSION})

	# C CXX Flags - turn on WARNINGS
    set (PROJECT_COMP_FLAGS "/W3")
    
elseif (BORLAND)
	message(STATUS "Compiler: BCC")
elseif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC) 
	message(STATUS "Compiler: GCC")
	
	# C CXX Flags - turn on WARNINGS
    set (PROJECT_COMP_FLAGS "-Wall -Wextra -pedantic -Wundef -Wcast-align -Wpointer-arith -Wformat")

	# C CXX Flags - turn off WARNINGS
    set (PROJECT_COMP_FLAGS "${PROJECT_COMP_FLAGS} -Wno-unused-parameter -Wno-parentheses -Wno-shadow -Wno-overloaded-virtual -Wno-ignored-qualifiers")
	
	# C CXX Flags - WARNINGS to ERRORS
    set (PROJECT_COMP_FLAGS "${PROJECT_COMP_FLAGS} -Werror=return-type -Werror=non-virtual-dtor")
    
	# C CXX Flags - turn on CODE GEN
    set (PROJECT_COMP_FLAGS "${PROJECT_COMP_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
    
	# C CXX Flags - turn off CODE GEN
    set (PROJECT_COMP_FLAGS "${PROJECT_COMP_FLAGS} -fno-exceptions -fno-check-new -fno-common -fno-threadsafe-statics")

    #Useful link: "http://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html"
else () 
	message (STATUS "Unknown compiler")
endif ()
message (STATUS "CMake generates " ${CMAKE_GENERATOR})


# Main settings
set (USE_GPROF NO)
set (GENERATE_INTERNAL_DOCS           NO CACHE BOOL "Generate internal documentation")
set (UNIT_TESTING                     NO CACHE BOOL "Compile binary executable for unit testing")
set (BUILD_MIME_SPEC                 YES CACHE BOOL "Build library with \"Shared MIME-info Database\"")
set (BUILD_DESKTOP_SPEC              YES CACHE BOOL "Build library with \"Desktop Entry Specification\"")
set (BUILD_THEMES_SPEC               YES CACHE BOOL "Build library with \"Icon Theme Specification\"")
set (BUILD_MENU_SPEC                 YES CACHE BOOL "Build library with \"Desktop Menu Specification\"")
set (BUILD_UPDATE_APPLICATIONS_CACHE YES CACHE BOOL "Build executable for rebuilding the cache")


# Custom modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
install (FILES ${CMAKE_SOURCE_DIR}/cmake/FindXDG.cmake DESTINATION share/cmake/Modules)


# Fix path delimeters
if (WIN32)
    file (TO_CMAKE_PATH ${CMAKE_INSTALL_PREFIX} CMAKE_INSTALL_PREFIX)
endif ()


# Set up compiler flags
set (CMAKE_CXX_FLAGS "${PROJECT_COMP_FLAGS}")


# Create config file
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

configure_file (${CMAKE_SOURCE_DIR}/src/config/config.h.in ${CMAKE_BINARY_DIR}/include/xdg/config.h @ONLY)
install (FILES ${CMAKE_BINARY_DIR}/include/xdg/config.h DESTINATION include/xdg)


# 3rdparty
find_package (Doxygen 1.7.5)
if (DOXYGEN_FOUND)
    # Config file variables
    set (DOXY_PROJECT_NAME   ${PROJECT_NAME})
    set (DOXY_PROJECT_NUMBER "0.2.1")
    set (DOXY_PROJECT_BRIEF  "\"This library is implementation of several freedesktop.org specifications.\"")
    set (DOXY_EXAMPLE_PATH   ${CMAKE_SOURCE_DIR}/doc)
    set (DOXY_INPUT          "${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/doc/doc_mainpage.h")
    
    if (GENERATE_INTERNAL_DOCS)
        set (DOXY_INTERNAL_DOCS "YES")
    else ()
        set (DOXY_INTERNAL_DOCS "NO")
    endif ()
    
    configure_file (${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)
    
    add_custom_target (doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doc/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
    
    install (DIRECTORY ${CMAKE_BINARY_DIR}/doc/ DESTINATION doc)
endif ()

if (BUILD_MENU_SPEC)
    find_package (LibXml2 REQUIRED 2.8.0)
    include_directories (${LIBXML2_INCLUDE_DIR})
    list (APPEND ${PROJECT_NAME}_LIBS ${LIBXML2_LIBRARIES})
    add_definitions (${LIBXML2_DEFINITIONS})
endif ()


# Includes
include_directories (${CMAKE_BINARY_DIR}/include)


# Link directories


# Libs
if (USE_GPROF)
    list (APPEND ${PROJECT_NAME}_LIBS gmon)
endif (USE_GPROF)


# Definitions


# Resources


# Compiler flags
if (USE_GPROF)
    set (CMAKE_CXX_FLAGS        ${CMAKE_CXX_FLAGS}        -pg)
    set (CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -pg)
endif (USE_GPROF)


# Targets
add_subdirectory (src)
