#include <Windows.h>
#include <thread>

#include "version.hpp"

#include "utility/logger/logger.hpp"
#include "modules/valorant/core.hpp"
#include "overlay/hook/hook.hpp"
#include "utility/event/event.hpp"

std::thread _loop_thread{};
HMODULE _chalkboard_handle{};

void
cleanup ()
{
  INFO ("Cleaning up");
  overlay::hook::uninstall ();
  core::unload ();
  utility::logger::close ();
}
bool
execute ()
{
  bool result{true};

  utility::logger::open (R"(C:\Users\ry\Desktop)",
			 utility::logger::level::kTRACE);
  INFO ("Chalkboard v" PROJECT_VERSION_STR " started");

  if (core::load ())
    {
      if (overlay::hook::install ())
	{
	  // This tells our module to quit,
	  // cleanup is handled inside
	  while (overlay::hook::is_rendering ())
	    {
	      PUBLISH (utility::event::update);
	      std::this_thread::sleep_for (std::chrono::seconds (1));
	    }
	}
      else
	{
	  ERROR ("Could not install DX11 hook");
	  result = false;
	}
    }
  else
    {
      ERROR ("Could not load core");
      result = false;
    }

  INFO ("Quitting");
  cleanup ();
  return result;
}

bool
try_execute ()
{
  bool result{};
  __try
    {
      result = execute ();
    }
  __except (1)
    {}
  return result;
}

void
entry ()
{
  if (!try_execute ())
    {
      MessageBoxA (NULL,
		   std::string (
		     "Chalkboard failed to run.\nLogs are saved on "
		     + utility::logger::get_path ()
		     + ".\nPlease send them to @asmxes on Discord :)")
		     .c_str (),
		   "Chalkboard", MB_OK | MB_ICONERROR);
    }
  FreeLibraryAndExitThread (_chalkboard_handle, 0);
}

BOOL WINAPI
DllMain (HMODULE hinstDLL,   // handle to DLL module
	 DWORD fdwReason,    // reason for calling function
	 LPVOID lpvReserved) // reserved
{
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      _chalkboard_handle = hinstDLL;
      DisableThreadLibraryCalls (_chalkboard_handle);

      _loop_thread = std::thread (entry);
      break;

    case DLL_PROCESS_DETACH:
      if (lpvReserved != nullptr)
	break; // do not do cleanup if process termination scenario

      cleanup ();
      if (_loop_thread.joinable ())
	_loop_thread.join ();
      break;

    default:
      break;
    }
  return TRUE;
}

extern "C" __declspec (dllexport) int
dummy_debug_proc (int code, WPARAM wParam, LPARAM lParam)
{
  return CallNextHookEx (NULL, code, wParam, lParam);
}