set(TARGET_NAME target)


add_executable(${TARGET_NAME} ${SOURCES})
# add_library(${TARGET_NAME} SHARED ${SOURCES})
# add_library(${TARGET_NAME} STATIC ${SOURCES})

# link_directories(${LIB_DIR})

# include_directories(${INCLUDE_DIR})

# target_link_libraries(${TARGET_NAME} libname)