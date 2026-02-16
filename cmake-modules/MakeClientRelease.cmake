add_custom_target(client-release
        DEPENDS
        injector chalkboard #loader
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:injector> ${CMAKE_SOURCE_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chalkboard> ${CMAKE_SOURCE_DIR}/bin/
        #        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:loader> ${CMAKE_SOURCE_DIR}/bin/b-rush.exe
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shared/config/.config ${CMAKE_SOURCE_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/version.txt ${CMAKE_SOURCE_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_SOURCE_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_SOURCE_DIR}/b-rush-v${PROJECT_VERSION}.zip" --format=zip -- .
        COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_SOURCE_DIR}/b-rush-v${PROJECT_VERSION}.zip" "${CMAKE_SOURCE_DIR}/bin/b-rush-v${PROJECT_VERSION}.zip"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
        COMMENT "Building client release bundle in /bin"
)