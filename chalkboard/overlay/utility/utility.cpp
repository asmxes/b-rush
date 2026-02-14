#include "utility.hpp"

namespace overlay {
namespace utility {
// Convert screen pos -> normalized offset from center
ImVec2
screen_to_normalized (const ImVec2 &pos)
{
  auto &io = ImGui::GetIO ();
  return {(pos.x - io.DisplaySize.x * 0.5f) / io.DisplaySize.x,
	  (pos.y - io.DisplaySize.y * 0.5f) / io.DisplaySize.y};
}

// Convert normalized offset -> screen pos
ImVec2
normalized_to_screen (const ImVec2 &norm)
{
  auto &io = ImGui::GetIO ();
  return {norm.x * io.DisplaySize.x + io.DisplaySize.x * 0.5f,
	  norm.y * io.DisplaySize.y + io.DisplaySize.y * 0.5f};
}
} // namespace utility
} // namespace overlay