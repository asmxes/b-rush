#ifndef CHALKBOARD_CORE_HPP
#define CHALKBOARD_CORE_HPP
#include <Windows.h>
#include <vector>
#include "imgui/imgui.h"

class core
{
  typedef std::vector<ImVec2> segment;
  std::vector<segment> _segments;

  bool _should_render;
  bool _draw_mode;
  bool _mouse_down;

  virtual void render ();
  virtual void render_menu ();
  virtual void wnd_proc (UINT msg, WPARAM wParam, LPARAM lParam);

public:
  core ();
  virtual bool wants_input ();
  virtual ~core ();
};

#endif