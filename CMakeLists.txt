cmake_minimum_required(VERSION 3.16)

project(timber-cpp VERSION 1.0
    DESCRIPTION "C++ wrapper for the timber C logging library"
    LANGUAGES C CXX)

include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CPP_EXAMPLES "Build examples" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(timber QUIET)

if(timber_FOUND)
    message(STATUS "Found existing timber library")
    set(TIMBER_FROM_SYSTEM TRUE)
else()
    message(STATUS "timber library not found, using submodule")
    set(TIMBER_FROM_SYSTEM FALSE)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/timber-c/.git")
        message(STATUS "Initializing timber-c submodule...")
        execute_process(
            COMMAND git submodule update --init --recursive timber-c
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT
        )
        if(NOT GIT_SUBMODULE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to initialize timber-c submodule")
        endif()
    endif()

    set(BUILD_EXAMPLES OFF CACHE BOOL "Disable timber-c examples" FORCE)
    add_subdirectory(timber-c)
endif()

# Create the interface library
add_library(timber-cpp INTERFACE)
add_library(timber-cpp::timber-cpp ALIAS timber-cpp)

target_include_directories(timber-cpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(timber-cpp INTERFACE timber)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

install(TARGETS timber-cpp
    EXPORT timber-cppTargets
)

install(EXPORT timber-cppTargets
    FILE timber-cppTargets.cmake
    NAMESPACE timber-cpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timber-cpp
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/timber-cppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/timber-cppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/timber-cppConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timber-cpp
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/timber-cppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/timber-cppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/timber-cpp
)

if(BUILD_CPP_EXAMPLES)
    add_subdirectory(examples)
endif()
