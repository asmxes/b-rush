#include "client.hpp"
#include "utility/logger/logger.hpp"

namespace api {
namespace ws {

client::~client ()
{
  INFO ("Disconnecting from websocket");
  disconnect ();
}

void
client::connect (const std::string &uri, const std::string &room,
		 const std::string &guid)
{
  if (_running)
    return;

  if (room.size () < 3)
    return;

  _room = room;
  _guid = guid;
  _running = true;

  // parse uri: ws://host:port
  // extract host and port
  std::string host_str = uri;
  int port = 80;

  // strip ws://
  if (host_str.find ("ws://") == 0)
    host_str = host_str.substr (5);

  auto colon = host_str.find (':');
  if (colon != std::string::npos)
    {
      port = std::stoi (host_str.substr (colon + 1));
      host_str = host_str.substr (0, colon);
    }

  std::wstring whost (host_str.begin (), host_str.end ());

  _thread = std::thread ([this, whost, port] () {
    HINTERNET session
      = WinHttpOpen (L"b-rush/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
      {
	ERROR ("Could not open session, error: {}", GetLastError ());
	_running = false;
	return;
      }
    DWORD timeout = 500; // 500ms
    if (!WinHttpSetTimeouts (session, 10000, 10000, 200, 200))
      {
	ERROR ("Could not set timeout, error: {}", GetLastError ());
      }

    HINTERNET connect = WinHttpConnect (session, whost.c_str (),
					static_cast<INTERNET_PORT> (port), 0);
    if (!connect)
      {
	ERROR ("Could not open connection, error: {}", GetLastError ());
	WinHttpCloseHandle (session);
	_running = false;
	return;
      }

    HINTERNET request
      = WinHttpOpenRequest (connect, L"GET", L"/", nullptr, WINHTTP_NO_REFERER,
			    WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!request)
      {
	ERROR ("Could not open request, error: {}", GetLastError ());
	WinHttpCloseHandle (connect);
	WinHttpCloseHandle (session);
	_running = false;
	return;
      }

    // upgrade to websocket
    if (!WinHttpSetOption (request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
			   nullptr, 0))
      {
	ERROR ("Could not upgrade to ws, error: {}", GetLastError ());
	WinHttpCloseHandle (request);
	WinHttpCloseHandle (connect);
	WinHttpCloseHandle (session);
	_running = false;
	return;
      }

    if (!WinHttpSendRequest (request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			     WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
      {
	ERROR ("Could not send request, error: {}", GetLastError ());
	WinHttpCloseHandle (request);
	WinHttpCloseHandle (connect);
	WinHttpCloseHandle (session);
	_running = false;
	return;
      }

    if (!WinHttpReceiveResponse (request, nullptr))
      {
	ERROR ("Could not receive response, error: {}", GetLastError ());
	WinHttpCloseHandle (request);
	WinHttpCloseHandle (connect);
	WinHttpCloseHandle (session);
	_running = false;
	return;
      }

    _websocket = WinHttpWebSocketCompleteUpgrade (request, 0);
    WinHttpCloseHandle (request);

    if (!_websocket)
      {
	ERROR ("Could not complete upgrade, error: {}", GetLastError ());
	WinHttpCloseHandle (connect);
	WinHttpCloseHandle (session);
	_running = false;
	return;
      }

    _connected = true;
    TRACE ("Connected to websocket");

    // send join
    nlohmann::json join_msg;
    join_msg["type"] = "join";
    join_msg["room"] = _room;
    join_msg["guid"] = _guid;
    send (join_msg);

    // receive loop
    std::vector<char> buf (4096);
    while (_running && _connected)
      {
	DWORD bytes_read = 0;
	WINHTTP_WEB_SOCKET_BUFFER_TYPE buf_type;

	auto err = WinHttpWebSocketReceive (_websocket, buf.data (),
					    static_cast<DWORD> (buf.size ()),
					    &bytes_read, &buf_type);

	if (err == ERROR_WINHTTP_TIMEOUT)
	  continue; // just loop back and check _running

	if (err != ERROR_SUCCESS)
	  {
	    _connected = false;
	    break;
	  }

	if (buf_type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
	  {
	    _connected = false;
	    break;
	  }

	if (buf_type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE
	    || buf_type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE)
	  {
	    std::string payload (buf.data (), bytes_read);
	    if (nlohmann::json::accept (payload))
	      {
		auto j = nlohmann::json::parse (payload, nullptr, false);
		// std::lock_guard<std::mutex> lk (_cb_mutex);
		//                        if (_on_message)
		//                          _on_message (j);
	      }
	    else
	      {
		WARNING ("Malformed payload received: {}", payload);
	      }
	  }
      }

    if (_websocket)
      {
	WinHttpWebSocketClose (_websocket,
			       WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr,
			       0);
	WinHttpCloseHandle (_websocket);
      }
    WinHttpCloseHandle (connect);
    WinHttpCloseHandle (session);

    _websocket = nullptr;
    _connected = false;
    _running = false;
  });
}

void
client::disconnect ()
{
  _running = false;

  if (_websocket && _connected)
    {
      WinHttpWebSocketClose (_websocket,
			     WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr,
			     0);
    }

  if (_thread.joinable ())
    _thread.join ();

  _connected = false;
}

void
client::send (const nlohmann::json &msg)
{
  if (!_connected || !_websocket)
    return;

  std::string payload = msg.dump ();
  std::lock_guard<std::mutex> lk (_send_mutex);
  WinHttpWebSocketSend (_websocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
			(void *) payload.data (),
			static_cast<DWORD> (payload.size ()));
}

client *
client::get ()
{
  static client obj;
  return &obj;
}

} // namespace ws
} // namespace api