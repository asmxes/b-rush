#ifndef CHALKBOARD_CORE_HPP
#define CHALKBOARD_CORE_HPP
#include <Windows.h>
#include <vector>
#include <string>
#include "imgui/imgui.h"

class core
{
  typedef std::vector<ImVec2> segment;
  std::vector<segment> _segments;

  bool _should_render;
  bool _draw_mode;
  bool _mouse_down;
  std::string _match_id;

  // TODO: Add on data callback
  void update ();
  void render ();
  void render_menu ();
  void wnd_proc (UINT msg, WPARAM wParam, LPARAM lParam);

public:
  core ();
  bool wants_input ();
  ~core ();
};

#endif