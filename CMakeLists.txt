cmake_minimum_required(VERSION 3.8)
project(JsonVariant VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT MSVC)
    add_compile_options(-W -Wextra -Wall -Werror -pedantic)
endif()

include(GNUInstallDirs) # Installation directories for `install` command
include(CMakePackageConfigHelpers)
include_directories("${CMAKE_SOURCE_DIR}/include")

option(BUILD_SHARED_LIBS "Build Shared Libraries (default OFF)" OFF) # static version is default
option(BUILD_EXAMPLES "Build and install examples (default OFF)" OFF)
option(BUILD_TESTS "Build tests (default OFF)" ON)

if (BUILD_SHARED_LIBS)
    set(LIB_TYPE SHARED)
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(LIB_TYPE STATIC)
endif()

add_library(jsonVariant ${LIB_TYPE} src/jsonVariant.cpp)
target_include_directories(jsonVariant PRIVATE include)

if (BUILD_EXAMPLES)
    add_subdirectory("examples")
endif()

if (BUILD_TESTS)
    add_subdirectory("test")
endif()

set_target_properties(PROPERTIES VERSION "${PROJECT_VERSION}")
install(FILES ${CMAKE_SOURCE_DIR}/include/jsonVariant.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_SOURCE_DIR}/include/jsonVariant.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS jsonVariant
    EXPORT jsonVariantTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin # For Windows DLLs
)

#installing cmake supported files
if(WIN32 AND NOT CYGWIN)
    set(JSON_VARIANT_CONFIG_INSTALL_DIR CMake)
else()
    set(JSON_VARIANT_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/jsonVariant)
endif()

configure_file(${CMAKE_SOURCE_DIR}/jsonVariantConfig.cmake.in jsonVariantConfig.cmake @ONLY)
write_basic_package_version_file(jsonVariantConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(EXPORT jsonVariantTargets
        FILE jsonVariantConfig.cmake
        NAMESPACE jsonVariant::
        DESTINATION ${JSON_VARIANT_CONFIG_INSTALL_DIR})

install(FILES ${CMAKE_BINARY_DIR}/jsonVariantConfigVersion.cmake DESTINATION ${JSON_VARIANT_CONFIG_INSTALL_DIR})
