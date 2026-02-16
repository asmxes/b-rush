#ifndef CHALKBOARD_OVERLAY_HOOK_HPP
#define CHALKBOARD_OVERLAY_HOOK_HPP

#include <Windows.h>

namespace overlay {
namespace hook {

bool
install ();

void
uninstall ();

HWND
get_hwnd ();

bool
is_rendering ();

bool
is_installed ();
} // namespace hook
} // namespace overlay
#endif