set(TARGET_NAME chalkboard)

add_library(${TARGET_NAME} SHARED
        main.cpp
        utility/event/event.cpp
        overlay/hook/hook.cpp
        overlay/draw/draw.cpp
        overlay/utility/utility.cpp
        utility/logger/logger.cpp
        utility/input/input.cpp
        core/core.cpp
)
sign_executable(${TARGET_NAME})
# add_library(${TARGET_NAME} STATIC ${SOURCES})

# link_directories(${LIB_DIR})

#include_directories(${INCLUDE_DIR})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${TARGET_NAME} PRIVATE imgui cpr::cpr nlohmann_json::nlohmann_json)
target_compile_options(${TARGET_NAME} PRIVATE /wd4005)