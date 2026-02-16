#include "api/riot.hpp"
#include "shared/logger/logger.hpp"
#include "api/ws.hpp"
#include "shared/config/config.hpp"

int
main ()
{
  logger::open (R"(%APPDATA%\b-rush\logs)", "chalkboard-test",
              config::get ("LOG_LEVEL", config::defaults::kLOG_LEVEL));

  while (1)
    {
      TRACE("HIII");
      std::this_thread::sleep_for (std::chrono::seconds (2));
    }
}