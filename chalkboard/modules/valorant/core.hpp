#ifndef CHALKBOARD_CORE_HPP
#define CHALKBOARD_CORE_HPP
#include <Windows.h>
#include <vector>
#include <string>
#include "imgui/imgui.h"

namespace core {
bool
load ();

void
unload ();

bool
wants_input ();
} // namespace core

#endif