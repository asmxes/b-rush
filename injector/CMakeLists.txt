set(TARGET_NAME injector)


add_executable(${TARGET_NAME} main.cpp)
sign_executable(${TARGET_NAME})

# add_library(${TARGET_NAME} SHARED ${SOURCES})
# add_library(${TARGET_NAME} STATIC ${SOURCES})

# link_directories(${LIB_DIR})

# include_directories(${INCLUDE_DIR})

# target_link_libraries(${TARGET_NAME} libname)