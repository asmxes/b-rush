#ifndef CHALKBOARD_OVERLAY_DRAW_COLOR_HPP
#define CHALKBOARD_OVERLAY_DRAW_COLOR_HPP

#define COL32_R_SHIFT 0
#define COL32_G_SHIFT 8
#define COL32_B_SHIFT 16
#define COL32_A_SHIFT 24

#include <sstream>
#include <iomanip>
#include "imgui/imgui.h"
#include "typedefs.hpp"

namespace overlay {

struct color
{
  u8 _r, _g, _b, _a;

  color () : _r (255), _g (255), _b (255), _a (255) {}
  color (u8 r, u8 g, u8 b, u8 a = 255) : _r (r), _g (g), _b (b), _a (a) {}
  color (f32 r, f32 g, f32 b, f32 a = 1.0f)
    : _r (static_cast<u8> (r * 255.0f)), _g (static_cast<u8> (g * 255.0f)),
      _b (static_cast<u8> (b * 255.0f)), _a (static_cast<u8> (a * 255.0f))
  {}

  color (const std::string &hex)
  {
    if (hex.empty () || hex[0] != '#'
	|| (hex.length () != 7 && hex.length () != 9))
      {
	_r = _g = _b = _a = 255;
	return;
      }

    auto hex_to_u8 = [] (const std::string &s) {
      return static_cast<u8> (std::stoul (s, nullptr, 16));
    };

    _r = hex_to_u8 (hex.substr (1, 2));
    _g = hex_to_u8 (hex.substr (3, 2));
    _b = hex_to_u8 (hex.substr (5, 2));
    _a = (hex.length () == 9) ? hex_to_u8 (hex.substr (7, 2)) : 255;
  }

  color (const color &other) = default;
  color (color &&other) noexcept = default;
  color &operator= (const color &other) = default;
  color &operator= (color &&other) noexcept = default;

  inline operator u32 () const
  {
    return (((u32) (_a) << COL32_A_SHIFT) | ((u32) (_b) << COL32_B_SHIFT)
	    | ((u32) (_g) << COL32_G_SHIFT) | ((u32) (_r) << COL32_R_SHIFT));
  }

  inline operator ImVec4 () const
  {
    return ImVec4 (_r / 255.0f, _g / 255.0f, _b / 255.0f, _a / 255.0f);
  }

  inline operator std::string () const
  {
    std::ostringstream oss;
    oss << '#' << std::hex << std::uppercase << std::setfill ('0')
	<< std::setw (2) << (u8) _r << std::setw (2) << (u8) _g << std::setw (2)
	<< (u8) _b << std::setw (2) << (u8) _a;

    return oss.str ();
  }
};
} // namespace overlay
#undef COL32_R_SHIFT
#undef COL32_G_SHIFT
#undef COL32_B_SHIFT
#undef COL32_A_SHIFT

#endif
