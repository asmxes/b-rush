#include "api/riot.hpp"
#include "utility/logger/logger.hpp"
#include "api/ws/ws.hpp"

int
main ()
{
  std::string state;
  logger::open (R"(C:\Users\ry\Desktop)", logger::level::kINFO);

  api::ws::client::get ()->connect (
    "ws://localhost:8765",
    api::riot::client::get ()->get_match_guid () + ":"
      + api::riot::client::get ()->get_team (),
    api::riot::client::get ()->get_player_guid ());

  while (1)
    {
      std::this_thread::sleep_for (std::chrono::seconds (2));
    }
}