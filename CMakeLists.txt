cmake_minimum_required(VERSION 3.8)
project(jsonVariant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
if(NOT MSVC)
    add_compile_options(-W -Wextra -Wall -Werror -pedantic)
endif()

include(GNUInstallDirs) # Installation directories for `install` command
include(CMakePackageConfigHelpers)

include_directories("${CMAKE_SOURCE_DIR}/include")

option(BUILD_EXAMPLES "Build and install examples (default OFF)" OFF)

set(JSON_VARIANT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

if (BUILD_EXAMPLES)
    add_subdirectory("examples")
endif()

#installing header
set_target_properties(PROPERTIES VERSION "${PROJECT_VERSION}")
install(FILES ${JSON_VARIANT_INCLUDE_DIR}/jsonVariant.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


#installing cmake supported files
if(WIN32 AND NOT CYGWIN)
    set(JSON_VARIANT_CONFIG_INSTALL_DIR CMake)
else()
    set(JSON_VARIANT_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/jsonVariant)
endif()

configure_file(${CMAKE_SOURCE_DIR}/jsonVariantConfig.cmake.in jsonVariantConfig.cmake @ONLY)
write_basic_package_version_file(jsonVariantConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_BINARY_DIR}/jsonVariantConfig.cmake ${CMAKE_BINARY_DIR}/jsonVariantConfigVersion.cmake DESTINATION ${JSON_VARIANT_CONFIG_INSTALL_DIR})
