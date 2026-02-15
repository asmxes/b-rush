#include "core.hpp"

#include <chrono>
#include <windowsx.h>

#include "utility/logger/logger.hpp"
#include "utility/event/event.hpp"
#include "utility/input/input.hpp"

#include "overlay/hook/hook.hpp"
#include "overlay/draw/draw.hpp"
#include "overlay/utility/utility.hpp"
#include "api/riot.hpp"
#include "api/ws.hpp"

namespace core {

typedef std::vector<ImVec2> segment;
std::vector<segment> _segments{};

bool _should_render{};
bool _draw_mode{};
bool _mouse_down{};
std::string _match_id{};

float _smooth{8};
float _thickness{4};
float _colorf[4]{1, 1, 1, 1};
ImVec2 _start{-0.131, -0.42}, end{-0.108, -0.41};
float _offsets[]{0, -0.034, -0.069, -0.1025, -0.137};

void
on_render ()
{
  static auto last = std::chrono::steady_clock::now ();
  auto now = std::chrono::steady_clock::now ();
  if (now - last >= std::chrono::milliseconds (5))
    {
      last = now;
      if (_match_id.empty ())
	return;
      if (_should_render)
	{
	  auto pos = utility::input::get_mouse_pos (overlay::hook::get_hwnd ());

	  if (utility::input::is_mouse_down (VK_RBUTTON))
	    {
	      auto &io = ImGui::GetIO ();

	      if (!_mouse_down)
		{
		  _mouse_down = true;
		  _segments.emplace_back ();
		  auto norm = overlay::utility::screen_to_normalized (pos);
		  _segments.back ().push_back (norm);
		  api::ws::send_start_draw (norm);
		}
	      else if (!_segments.empty () && !_segments.back ().empty ())
		{
		  auto norm = overlay::utility::screen_to_normalized (pos);
		  auto &last = _segments.back ().back ();
		  float dx = (norm.x - last.x) * io.DisplaySize.x;
		  float dy = (norm.y - last.y) * io.DisplaySize.y;
		  if (dx * dx + dy * dy >= 25.0f)
		    _segments.back ().push_back (norm);
		  api::ws::send_add_point (norm);
		}
	    }
	  else
	    {
	      if (_mouse_down)
		{
		  api::ws::send_end_draw ();
		  _mouse_down = false;
		}
	    }
	}
    }

  if (_should_render)
    {
      for (auto &seg : _segments)
	{
	  if (seg.size () < 2)
	    continue;

	  std::vector<ImVec2> screen_points;
	  screen_points.reserve (seg.size ());
	  for (const auto &p : seg)
	    screen_points.push_back (
	      overlay::utility::normalized_to_screen (p));

	  overlay::draw::path (screen_points, _smooth, _thickness,
			       overlay::color (_colorf[0], _colorf[1],
					       _colorf[2], _colorf[3]));
	}

      // Convert to screen for drawing
      for (int i = 0; i < ARRAYSIZE (_offsets); i++)
	{
	  ImVec2 start_screen = overlay::utility::normalized_to_screen (
	    {_start.x + _offsets[i], _start.y});
	  ImVec2 end_screen = overlay::utility::normalized_to_screen (
	    {end.x + _offsets[i], end.y});
	  overlay::draw::rectangle (start_screen, end_screen, true,
				    overlay::color (_colorf[0], _colorf[1],
						    _colorf[2], _colorf[3]));
	}
    }
}

void
on_render_menu ()
{
  ImGui::Begin ("Chalkboard", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    ImGui::SliderFloat ("Smooth", &_smooth, 1.0f, 20.0f);
    ImGui::SliderFloat ("Thickness", &_thickness, 0.0f, 10.0f);
    ImGui::ColorEdit4 ("Color", _colorf, ImGuiColorEditFlags_AlphaBar);
  }
  ImGui::End ();
}

void
on_wnd_proc (UINT msg, WPARAM wparam, LPARAM lParam)
{
  if (msg == WM_KEYDOWN)
    {
      switch (wparam)
	{
	case VK_CAPITAL:
	  _should_render = true;
	  break;
	case 'D':
	  if (_should_render)
	    _draw_mode = !_draw_mode;
	  break;
	case 'E':
	  if (_should_render)
	    {
	      if (!_segments.empty ())
		_segments.clear ();
	      api::ws::send_erase ();
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
	  _should_render = false;
	  _draw_mode = false;
	}
    }
}

bool
wants_input ()
{
  return false;
  // return _should_render && _draw_mode;
}

void
on_update ()
{
  auto current_match = api::riot::get_match_guid ();
  if (_match_id != current_match)
    {
      INFO ("Match changed, new: {}, old: {}", _match_id, current_match);
      _match_id = current_match;

      api::ws::disconnect ();
      if (!_segments.empty ())
	_segments.clear ();

      if (!_match_id.empty ())
	{
	  auto team = api::riot::get_team ();
	  api::ws::connect ("ws.b-rush.com", _match_id + ":" + team,
			    api::riot::get_player_guid ());
	}
    }

  // TODO: send ws command for new segments
}

void
handle_joined (nlohmann::json *json)
{}

void
handle_peer_joined (nlohmann::json *json)
{}

void
handle_start_draw (nlohmann::json *json)
{}

void
handle_add_point (nlohmann::json *json)
{}

void
handle_end_draw (nlohmann::json *json)
{}

void
handle_erase (nlohmann::json *json)
{}

void
on_ws_data (nlohmann::json *json)
{
  INFO ("{}", json->dump ());
}

bool
load ()
{
  if (!api::riot::connect ())
    {
      ERROR ("Could not install DX11 hook");
      return false;
    }

  INFO ("Logged in as: {}, GUID: {}", api::riot::get_riot_id (),
	api::riot::get_player_guid ());

  if (!api::ws::connect ("ws.b-rush.com", "heart:beat",
			 api::riot::get_player_guid ()))
    {
      ERROR ("Could not connect to ws");
      return false;
    }

  // We dont care about the heartbeat connect, we use it only to make sure the
  // server is available
  api::ws::disconnect ();

  SUBSCRIBE (utility::event::id::update, &core::on_update);
  SUBSCRIBE (utility::event::id::render, &core::on_render);
  SUBSCRIBE (utility::event::id::render_menu, &core::on_render_menu);
  SUBSCRIBE (utility::event::id::wnd_proc, &core::on_wnd_proc);
  SUBSCRIBE (utility::event::id::ws_data, &core::on_ws_data);
  return true;
}

void
unload ()
{
  TRACE ("Requested module unload");

  INFO ("UnSubscribing to events");
  UNSUBSCRIBE (utility::event::id::update, &core::on_update);
  UNSUBSCRIBE (utility::event::id::render, &core::on_render);
  UNSUBSCRIBE (utility::event::id::render_menu, &core::on_render_menu);
  UNSUBSCRIBE (utility::event::id::wnd_proc, &core::on_wnd_proc);
  UNSUBSCRIBE (utility::event::id::ws_data, &core::on_ws_data);

  api::ws::disconnect ();
  // api::riot::disconnect();
  TRACE ("Completed module unload");
}
} // namespace core