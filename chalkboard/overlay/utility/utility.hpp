#ifndef CHALKBOARD_OVERLAY_UTILITY_HPP
#define CHALKBOARD_OVERLAY_UTILITY_HPP

#include "imgui/imgui.h"

namespace overlay {
namespace utility {
ImVec2
screen_to_normalized (const ImVec2 &pos);

ImVec2
normalized_to_screen (const ImVec2 &norm);
} // namespace utility
} // namespace overlay

#endif
