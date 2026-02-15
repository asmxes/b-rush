#ifndef CHALKBOARD_OVERLAY_HOOK_HPP
#define CHALKBOARD_OVERLAY_HOOK_HPP

#include "core/core.hpp"

namespace overlay {
class hook
{
  hook ();

public:
  static hook *get ();
  ~hook ();

  HWND get_hwnd ();
  bool is_rendering ();
};
} // namespace overlay
#endif