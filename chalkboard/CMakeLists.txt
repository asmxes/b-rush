set(TARGET_NAME chalkboard)

set(SOURCES
        main.cpp
        utility/event/event.cpp
        overlay/hook/hook.cpp
        overlay/draw/draw.cpp
        overlay/utility/utility.cpp
        utility/logger/logger.cpp
        utility/input/input.cpp
        utility/generic.cpp

        api/riot/client.cpp
        api/ws/client.cpp

        core/core.cpp
)
add_library(${TARGET_NAME} SHARED ${SOURCES})
sign_executable(${TARGET_NAME})
# add_library(${TARGET_NAME} STATIC ${SOURCES})

# link_directories(${LIB_DIR})

#include_directories(${INCLUDE_DIR})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${TARGET_NAME} PRIVATE imgui cpr::cpr nlohmann_json::nlohmann_json)
target_compile_options(${TARGET_NAME} PRIVATE /wd4005)

# test executable
add_executable(chalkboard-test ${SOURCES} test.cpp)
target_include_directories(chalkboard-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chalkboard-test PRIVATE imgui cpr::cpr nlohmann_json::nlohmann_json)