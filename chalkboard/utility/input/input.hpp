#ifndef CHALKBOARD_UTILITY_INPUT_HPP
#define CHALKBOARD_UTILITY_INPUT_HPP

#include <Windows.h>

#include "imgui/imgui.h"

namespace utility {
namespace input {

bool
is_mouse_down (int button = VK_LBUTTON);

ImVec2
get_mouse_pos (HWND hwnd);

} // namespace input
} // namespace utility

#endif
