#include "core.hpp"

#include <chrono>
#include <windowsx.h>

#include "shared/logger/logger.hpp"
#include "utility/event/event.hpp"
#include "utility/input/input.hpp"

#include "overlay/hook/hook.hpp"
#include "overlay/draw/draw.hpp"
#include "overlay/utility/utility.hpp"
#include "api/riot.hpp"
#include "api/ws.hpp"
#include "config/config.hpp"

#include "version.hpp"

namespace core {

typedef std::vector<ImVec2> segment;
std::vector<segment> _segments{};

struct peer
{
  overlay::color color;
  std::vector<segment> segments;
};

std::mutex _peers_mutex{};
std::map<std::string, peer> _peers{};

bool _should_render{};
bool _draw_mode{};
bool _mouse_down{};
std::string _match_id{};
std::string _room_override{};

float _smooth{4};
float _thickness{4};
overlay::color _color{};

// Used to draw colors under agent potrait
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

	  if (utility::input::is_key_down (VK_RBUTTON))
	    {
	      auto &io = ImGui::GetIO ();

	      if (!_mouse_down)
		{
		  _mouse_down = true;
		  _segments.emplace_back ();
		  auto norm = overlay::utility::screen_to_normalized (pos);
		  // ue::unrotate(norm)
		  _segments.back ().push_back (norm);
		  api::ws::send_start_draw (norm);
		}
	      else if (!_segments.empty () && !_segments.back ().empty ())
		{
		  auto norm = overlay::utility::screen_to_normalized (pos);
		  // ue::unrotate(norm)
		  auto &last = _segments.back ().back ();
		  float dx = (norm.x - last.x) * io.DisplaySize.x;
		  float dy = (norm.y - last.y) * io.DisplaySize.y;
		  if (dx * dx + dy * dy >= 25.0f)
		    {
		      _segments.back ().push_back (norm);
		      api::ws::send_add_point (norm);
		    }
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
      // Own segments
      for (auto &seg : _segments)
	{
	  if (seg.size () < 2)
	    continue;

	  std::vector<ImVec2> screen_points;
	  screen_points.reserve (seg.size ());
	  for (const auto &p : seg)
	    // ue::rotate(p)
	    screen_points.push_back (
	      overlay::utility::normalized_to_screen (p));

	  overlay::draw::path (screen_points, _smooth, _thickness, _color);
	}

      // Teammates segments
      std::lock_guard<std::mutex> lk (_peers_mutex);
      for (const auto &[guid, p] : _peers)
	{
	  for (const auto &seg : p.segments)
	    {
	      if (seg.size () < 2)
		continue;

	      std::vector<ImVec2> screen_points;
	      screen_points.reserve (seg.size ());
	      for (const auto &pt : seg)
		// ue::rotate(p)
		screen_points.push_back (
		  overlay::utility::normalized_to_screen (pt));

	      overlay::draw::path (screen_points, _smooth, _thickness, p.color);
	    }
	}

      // Convert to screen for drawing
      //      for (int i = 0; i < ARRAYSIZE (_offsets); i++)
      // {
      //   ImVec2 start_screen = overlay::utility::normalized_to_screen (
      //     {_start.x + _offsets[i], _start.y});
      //   ImVec2 end_screen = overlay::utility::normalized_to_screen (
      //     {end.x + _offsets[i], end.y});
      //   overlay::draw::rectangle (start_screen, end_screen, true,
      // 			    overlay::color (_colorf[0], _colorf[1],
      // 					    _colorf[2], _colorf[3]));
      // }
    }
}

void
on_render_menu ()
{
  auto start_pos = overlay::utility::normalized_to_screen ({-0.492, -0.450});
  overlay::draw::text (start_pos, "b-rush v" PROJECT_VERSION_STR, false, false,
		       overlay::color ("#ffffff44"));
  if (_match_id.empty ())
    {
      overlay::draw::text ({start_pos.x, start_pos.y + 10},
			   "Waiting for match...", false, false,
			   overlay::color ("#ffff00"));
    }
  else
    {
      auto text = _room_override.empty () ? "match room" : "override room";
      overlay::draw::text ({start_pos.x, start_pos.y + 10},
			   std::format ("Connected to {}", text).c_str (),
			   false, false, overlay::color ("#00ff00"));
    }

#ifdef _DEBUG
  ImGui::Begin ("Debug");
  {
    ImGui::Text (std::format ("match id: {}", _match_id).c_str ());
    ImGui::Text (std::format ("room override: {}", _room_override).c_str ());
    for (auto &peer : _peers)
      {
	ImGui::Text (std::format ("peer: {}, color: {}", peer.first,
				  std::string (peer.second.color))
		       .c_str ());
      }
  }
  ImGui::End ();
#endif
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
	      if (utility::input::is_key_down (VK_LCONTROL))
		{
		  std::lock_guard<std::mutex> lk (_peers_mutex);
		  for (auto &peer : _peers)
		    {
		      peer.second.segments.clear ();
		    }
		}
	      else
		{
		  if (!_segments.empty ())
		    _segments.clear ();
		  api::ws::send_erase ();
		}
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
	  auto room
	    = _room_override.empty () ? _match_id + ":" + team : _room_override;
	  api::ws::connect (config::get ("WS_URI", config::defaults::kWS_URI),
			    room, api::riot::get_player_guid ());
	}
    }

  // TODO: send ws command for new segments (? to be cheked because right now it
  // works fine on_draw)
  // TODO: ue::round_has_restarted() check
}

void
handle_joined (nlohmann::json *json)
{
  auto c = (*json)["color"];
  _color = overlay::color (c[0].get<u8> (), c[1].get<u8> (), c[2].get<u8> (),
			   c[3].get<u8> ());
}

void
handle_peer_joined (nlohmann::json *json)
{
  auto guid = (*json)["guid"].get<std::string> ();
  auto c = (*json)["color"];
  auto color = overlay::color (c[0].get<u8> (), c[1].get<u8> (),
			       c[2].get<u8> (), c[3].get<u8> ());

  std::lock_guard<std::mutex> lk (_peers_mutex);
  _peers[guid].color = color;
}

void
handle_start_draw (nlohmann::json *json)
{
  auto guid = (*json)["guid"].get<std::string> ();
  float x = (*json)["x"].get<float> ();
  float y = (*json)["y"].get<float> ();

  std::lock_guard<std::mutex> lk (_peers_mutex);
  if (!_peers.contains (guid))
    return;

  _peers[guid].segments.emplace_back ();
  _peers[guid].segments.back ().push_back ({x, y});
}

void
handle_add_point (nlohmann::json *json)
{
  auto guid = (*json)["guid"].get<std::string> ();
  float x = (*json)["x"].get<float> ();
  float y = (*json)["y"].get<float> ();

  std::lock_guard<std::mutex> lk (_peers_mutex);
  if (!_peers.contains (guid))
    return;

  auto &segs = _peers[guid].segments;
  if (!segs.empty ())
    segs.back ().push_back ({x, y});
}

void
handle_end_draw (nlohmann::json *json)
{}

void
handle_erase (nlohmann::json *json)
{
  auto guid = (*json)["guid"].get<std::string> ();

  std::lock_guard<std::mutex> lk (_peers_mutex);
  if (_peers.contains (guid))
    _peers[guid].segments.clear ();
}

void
handle_peer_left (nlohmann::json *json)
{
  auto guid = (*json)["guid"].get<std::string> ();

  std::lock_guard<std::mutex> lk (_peers_mutex);
  _peers.erase (guid);
}

void
on_ws_data (nlohmann::json *json)
{
  try
    {
      auto type = (*json)["type"].get<std::string> ();

      if (type == "joined")
	handle_joined (json);
      else if (type == "peer_joined")
	handle_peer_joined (json);
      else if (type == "start_draw")
	handle_start_draw (json);
      else if (type == "add_point")
	handle_add_point (json);
      else if (type == "end_draw")
	handle_end_draw (json);
      else if (type == "erase")
	handle_erase (json);
      else if (type == "peer_left")
	handle_peer_left (json);
    }
  catch (std::exception &e)
    {
      WARNING ("Exception thrown: {}", e.what ());
    }
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

  if (!api::ws::connect (config::get ("WS_URI", config::defaults::kWS_URI),
			 "heart:beat", api::riot::get_player_guid ()))
    {
      ERROR ("Could not connect to ws");
      return false;
    }

  // We dont care about the heartbeat connect, we use it only to make sure the
  // server is available
  api::ws::disconnect ();

  _room_override
    = config::get ("ROOM_OVERRIDE", config::defaults::kROOM_OVERRIDE);
  if (!_room_override.empty ())
    {
      INFO ("Room override: {}", _room_override);
    }

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