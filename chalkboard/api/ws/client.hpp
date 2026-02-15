#ifndef CHALKBOARD_API_WS_CLIENT_HPP
#define CHALKBOARD_API_WS_CLIENT_HPP

#include <string>
#include <thread>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <mutex>

#include <nlohmann/json.hpp>

namespace api {
namespace ws {
class client
{
  HINTERNET _websocket;
  std::string _room;
  std::string _guid;
  bool _connected;
  bool _running;
  std::thread _thread;
  std::mutex _send_mutex;

  client () {}
  void send (const nlohmann::json &msg);

public:
  ~client ();
  void connect (const std::string &uri, const std::string &room,
		const std::string &guid);
  void disconnect ();
  static client *get ();
};
} // namespace ws
} // namespace api

#endif
