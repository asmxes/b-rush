set(TARGET_NAME injector)


add_executable(${TARGET_NAME} main.cpp)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    sign_executable(${TARGET_NAME})
endif ()

# add_library(${TARGET_NAME} SHARED ${SOURCES})
# add_library(${TARGET_NAME} STATIC ${SOURCES})

# link_directories(${LIB_DIR})

# include_directories(${INCLUDE_DIR})

# target_link_libraries(${TARGET_NAME} libname)