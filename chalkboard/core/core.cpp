#include "core.hpp"

#include <chrono>
#include <windowsx.h>

#include "utility/logger/logger.hpp"
#include "utility/event/event.hpp"
#include "utility/input/input.hpp"

#include "overlay/hook/hook.hpp"
#include "overlay/draw/draw.hpp"
#include "overlay/utility/utility.hpp"

float smooth = 8;
float thickness = 4;
float colorf[4]{1, 1, 1, 1};
ImVec2 start{-0.131, -0.42}, end{-0.108, -0.41};
float offsets[]{0, -0.034, -0.069, -0.1025, -0.137};

void
core::render ()
{
  static auto last = std::chrono::steady_clock::now ();
  auto now = std::chrono::steady_clock::now ();
  if (now - last >= std::chrono::milliseconds (5))
    {
      last = now;
      if (this->_should_render)
	{
	  auto pos = utility::input::get_mouse_pos (
	    overlay::hook::get ()->get_hwnd ());

	  if (utility::input::is_mouse_down (VK_RBUTTON))
	    {
	      auto &io = ImGui::GetIO ();

	      if (!this->_mouse_down)
		{
		  this->_mouse_down = true;
		  this->_segments.emplace_back ();
		  this->_segments.back ().push_back (
		    overlay::utility::screen_to_normalized (pos));
		}
	      else if (!this->_segments.empty ()
		       && !this->_segments.back ().empty ())
		{
		  auto norm = overlay::utility::screen_to_normalized (pos);
		  auto &last = this->_segments.back ().back ();
		  float dx = (norm.x - last.x) * io.DisplaySize.x;
		  float dy = (norm.y - last.y) * io.DisplaySize.y;
		  if (dx * dx + dy * dy >= 25.0f)
		    this->_segments.back ().push_back (norm);
		}
	    }
	  else
	    {
	      this->_mouse_down = false;
	    }
	}
    }

  if (this->_should_render)
    {
      for (auto &seg : this->_segments)
	{
	  if (seg.size () < 2)
	    continue;

	  std::vector<ImVec2> screen_points;
	  screen_points.reserve (seg.size ());
	  for (const auto &p : seg)
	    screen_points.push_back (
	      overlay::utility::normalized_to_screen (p));

	  overlay::draw::path (screen_points, smooth, thickness,
			       overlay::color (colorf[0], colorf[1], colorf[2],
					       colorf[3]));
	}

      // Convert to screen for drawing
      for (int i = 0; i < ARRAYSIZE (offsets); i++)
	{
	  ImVec2 start_screen = overlay::utility::normalized_to_screen (
	    {start.x + offsets[i], start.y});
	  ImVec2 end_screen = overlay::utility::normalized_to_screen (
	    {end.x + offsets[i], end.y});
	  overlay::draw::rectangle (start_screen, end_screen, true,
				    overlay::color (colorf[0], colorf[1],
						    colorf[2], colorf[3]));
	}
    }
}

void
core::render_menu ()
{
  overlay::draw::text ({200, 200},
		       "Should render: "
			 + std::string (this->_should_render ? "TRUE"
							     : "FALSE"),
		       false, true);
  overlay::draw::text ({200, 210},
		       "Draw mode: "
			 + std::string (this->_draw_mode ? "ON" : "OFF"),
		       false, true);
  overlay::draw::text ({200, 220},
		       "Mouse: "
			 + std::string (this->_mouse_down ? "PRESSED"
							  : "NOT PRESSED"),
		       false, true);
  overlay::draw::text ({200, 230},
		       "Segments: " + std::to_string (this->_segments.size ()),
		       false, true);

  ImGui::Begin ("Chalkboard", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    ImGui::SliderFloat ("Smooth", &smooth, 1.0f, 20.0f);
    ImGui::SliderFloat ("Thickness", &thickness, 0.0f, 10.0f);
    ImGui::ColorEdit4 ("Color", colorf, ImGuiColorEditFlags_AlphaBar);
  }
  ImGui::End ();
}

void
core::wnd_proc (UINT msg, WPARAM wparam, LPARAM lParam)
{
  // TRACE ("DX11 WNDPROC called, msg: " + std::to_string (msg) + ", wparam: "
  // + std::to_string (wparam) + ", lparam: " + std::to_string (lParam));

  if (msg == WM_KEYDOWN)
    {
      switch (wparam)
	{
	case VK_CAPITAL:
	  this->_should_render = true;
	  break;
	case 'D':
	  if (this->_should_render)
	    this->_draw_mode = !this->_draw_mode;
	  break;
	case 'E':
	  if (this->_should_render)
	    {
	      if (!this->_segments.empty ())
		this->_segments.clear ();
	    }
	  break;
	default:
	  break;
	}
    }
  else if (msg == WM_KEYUP)
    {
      if (wparam == VK_CAPITAL)
	{
	  this->_should_render = false;
	  this->_draw_mode = false;
	}
    }
}

bool
core::wants_input ()
{
  return false;
  // return this->_should_render && this->_draw_mode;
}

core::core ()
{
  this->_should_render = false;
  this->_draw_mode = false;
  this->_mouse_down = false;
  this->_segments.clear ();

  INFO ("Subscribing to events");
  SUBSCRIBE (utility::event::id::render, this, &core::render);
  SUBSCRIBE (utility::event::id::render_menu, this, &core::render_menu);
  SUBSCRIBE (utility::event::id::wnd_proc, this, &core::wnd_proc);
}

core::~core ()
{
  INFO ("UnSubscribing to events");
  UNSUBSCRIBE (utility::event::id::render, this);
  UNSUBSCRIBE (utility::event::id::render_menu, this);
  UNSUBSCRIBE (utility::event::id::wnd_proc, this);
}
