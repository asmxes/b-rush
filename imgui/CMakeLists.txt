add_library(imgui STATIC
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        imgui_impl_dx11.cpp
        imgui_impl_win32.cpp
)

add_executable(binary_to_compressed_c binary_to_compressed_c.cpp)

target_include_directories(imgui PUBLIC .)