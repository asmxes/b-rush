#ifndef CHALKBOARD_OVERLAY_DRAW_HPP
#define CHALKBOARD_OVERLAY_DRAW_HPP

#include <string>
#include <vector>

#include "color.hpp"
#include "imgui/imgui.h"

namespace overlay {
namespace draw {
void
text (const ImVec2 &pos, const std::string &text, bool centered = true,
      bool outline = false, color col = color ());
void
rectangle (const ImVec2 &start, const ImVec2 &end, bool filled = false,
	   color col = color ());
void
circle (const ImVec2 &center, float radius, bool filled = false,
	float thickness = 1.f, color col = color ());

void
line (const ImVec2 &start, const ImVec2 &end, float thickness = 1.f,
      color col = color ());

void
path (const std::vector<ImVec2> &points, float smooth = 1.f,
      float thickness = 1.f, color col = color ());

} // namespace draw
} // namespace overlay

#endif
