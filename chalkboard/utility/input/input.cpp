#include "input.hpp"

namespace utility {
namespace input {
bool
is_mouse_down (int button)
{
  return GetAsyncKeyState (button) & 0x8000;
}

ImVec2
get_mouse_pos (HWND hwnd)
{
  POINT p;
  GetCursorPos (&p);
  ScreenToClient (hwnd, &p);
  return {static_cast<float> (p.x), static_cast<float> (p.y)};
}
} // namespace input
} // namespace utility