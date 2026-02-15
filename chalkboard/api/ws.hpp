#ifndef CHALKBOARD_API_WS_HPP
#define CHALKBOARD_API_WS_HPP

#include <string>
#include <thread>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <mutex>

#include <nlohmann/json.hpp>
#include "imgui/imgui.h"

namespace api {
namespace ws {
bool
connect (const std::string &uri, const std::string &room,
	 const std::string &guid);

void
disconnect ();

void
send_draw (const std::vector<ImVec2> &path);

void
send_start_draw (const ImVec2 &point);

void
send_add_point (const ImVec2 &point);

void
send_end_draw ();

void
send_erase ();
} // namespace ws
} // namespace api

#endif
