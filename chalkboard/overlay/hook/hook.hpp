#ifndef CHALKBOARD_OVERLAY_HOOK_HPP
#define CHALKBOARD_OVERLAY_HOOK_HPP

#include "core/core.hpp"

namespace overlay {
class hook
{
  hook ();

public:
  static hook *get ();
  virtual ~hook ();

  virtual HWND get_hwnd ();
  virtual bool is_rendering ();
};
} // namespace overlay
#endif