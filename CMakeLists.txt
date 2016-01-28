cmake_minimum_required (VERSION 2.4)


# Custom modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")


# Platform
include (platform)
initialize_platform ()


# Install
include (platform/install)


# Tests
include (platform/tests)
add_project_tests_main_target ()


# Documentation
include (platform/doxygen)
add_project_documentation_main_target ()


# Platform definition
include (platform/options)
platform_options_begin (cmake/platform_options.cmake)
    platform_option_value (DE_KDE                           ON)
    platform_option_value (ENABLE_POSITION_INDEPENDENT_CODE ON)
platform_options_end ()


# Project - xdg
add_subdirectory (src)
