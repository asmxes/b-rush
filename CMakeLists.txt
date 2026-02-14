cmake_minimum_required(VERSION 4.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")
include(Sign)

SET(V_MAJ0R 1)
SET(V_MINOR 0)
SET(V_PATCH 0)
SET(V_REVISION 1)

project(b-rush
        VERSION ${V_MAJ0R}.${V_MINOR}.${V_PATCH}.${V_REVISION}
        DESCRIPTION "My Project Description")

if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "Build type not specified. Setting default ('Debug')")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(ARCH "x64")
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules
        CACHE STRING "CMake module path" FORCE)

set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.14.1) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

configure_file("${PROJECT_SOURCE_DIR}/version.hpp.in"
        "${PROJECT_SOURCE_DIR}/version.hpp" @ONLY)

add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Zi>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/shared
        "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Include"
        "C:\\Users\\ry\\source\\repos\\vcpkg\\installed\\x64-windows\\include"
)

link_directories(
        ${LIB_DIR}
        "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x64"
        "C:\\Users\\ry\\source\\repos\\vcpkg\\installed\\x64-windows\\lib"
)

#include(cmake_module_name)
add_subdirectory(imgui)

add_subdirectory(server)
add_subdirectory(chalkboard)
add_subdirectory(injector)
#add_subdirectory(loader)
