cmake_minimum_required(VERSION 3.1O)

SET(V_MAJ0R 1)
SET(V_MINOR 0)
SET(V_PATCH 0)
SET(V_REVISION 1)

project(my_project
        VERSION ${V_MAJ0R}.${V_MINOR}.${V_PATCH}.${V_REVISION}
        DESCRIPTION "My Project Description")

if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "Build type not specified. Setting default ('Debug')")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif ()

set(CMAKE_CXX_STANDARD 11)
set(ARCH "x64")
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules
        CACHE STRING "CMake module path" FORCE)

configure_file("${PROJECT_SOURCE_DIR}/version.hpp.in"
        "${PROJECT_SOURCE_DIR}/version.hpp" @ONLY)

add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Zi>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")

include_directories(${PROJECT_SOURCE_DIR})

# link_directories(${LIB_DIR})


include(cmake_module_name)
add_subdirectory(src)
