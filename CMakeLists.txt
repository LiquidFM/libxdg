cmake_minimum_required (VERSION 2.8)
project (exdgmime)


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
set (USE_GPROF )


# Custom modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")


# Fix path delimeters
if (WIN32)
    file (TO_CMAKE_PATH ${CMAKE_INSTALL_PREFIX} CMAKE_INSTALL_PREFIX)
endif ()


# Set up compiler flags
set (CMAKE_CXX_FLAGS "${PROJECT_COMP_FLAGS}")


# 3rdparty
find_package (Doxygen 1.7.5)
if (DOXYGEN_FOUND)
    # Config file variables
    set (DOXY_PROJECT_NAME   ${PROJECT_NAME})
    set (DOXY_PROJECT_NUMBER "1")
    set (DOXY_PROJECT_BRIEF  "\"This library is a fork of the official freedesktop.org library xdgmime.\"")
    set (DOXY_INPUT          ${CMAKE_SOURCE_DIR}/src)
    
    configure_file (${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)
    
    add_custom_target (doc
        ALL
        ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doc/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif ()

# Includes

# Link directories

# Libs
if (USE_GPROF)
	set (exdgmime_LIBS ${exdgmime_LIBS} gmon)
endif (USE_GPROF)
 
# Definitions
add_definitions (-DHAVE_MMAP)

# Sources
add_subdirectory("src")

# Resources

# Compiler flags
if (USE_GPROF)
    set (CMAKE_CXX_FLAGS        ${CMAKE_CXX_FLAGS}        -pg)
    set (CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} -pg)
endif (USE_GPROF)

# Target - update-applications-cache
add_executable (update-applications-cache ${exdgmime_SOURCES})
target_link_libraries (update-applications-cache ${exdgmime_LIBS})

# Install
include ("cmake/Install.cmake")
