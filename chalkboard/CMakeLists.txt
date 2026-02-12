set(TARGET_NAME chalkboard)


add_library(${TARGET_NAME} SHARED
        main.cpp
        overlay/overlay.cpp
        logger/logger.cpp
)
sign_executable(${TARGET_NAME})
# add_library(${TARGET_NAME} STATIC ${SOURCES})

# link_directories(${LIB_DIR})

# include_directories(${INCLUDE_DIR})

target_link_libraries(${TARGET_NAME} PRIVATE imgui)
target_compile_options(${TARGET_NAME} PRIVATE /wd4005)