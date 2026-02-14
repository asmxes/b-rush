#include <Windows.h>
#include <thread>

#include "version.hpp"

#include "utility/logger/logger.hpp"
#include "core/core.hpp"
#include "overlay/hook/hook.hpp"

std::thread loop_thread;
HMODULE chalkboard_handle;

bool
execute ()
{
  MessageBoxA (NULL, "Hi", "Chalkboard", MB_OK);

  auto core_instance = core ();

  while (overlay::hook::get ()->is_rendering ())
    {
      // TODO: Send heartbeat? or poll data from server idk
      std::this_thread::sleep_for (std::chrono::seconds (1));
    }

  MessageBoxA (NULL, "Bye", "Chalkboard", MB_OK);
  return true;
}

bool
thread ()
{
  bool result;
  __try
    {
      result = execute ();
    }
  __except (1)
    {
      result = false;
    }
  return result;
}

void
entry ()
{
  logger::get ()->open (R"(C:\Users\ry\Desktop)", logger::level::kTRACE);
  INFO ("Chalkboard V" PROJECT_VERSION_STR " started");

  if (!thread ())
    MessageBoxA (NULL,
		 std::string ("Chalkboard failed to run.\nLogs are saved on "
			      + logger::get ()->get_path ()
			      + ".\nPlease send them to @asmxes on Discord :)")
		   .c_str (),
		 "Chalkboard", MB_OK | MB_ICONERROR);

  INFO ("Quitting");
  FreeLibraryAndExitThread (chalkboard_handle, 0);
}

BOOL WINAPI
DllMain (HMODULE hinstDLL,   // handle to DLL module
	 DWORD fdwReason,    // reason for calling function
	 LPVOID lpvReserved) // reserved
{
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      chalkboard_handle = hinstDLL;
      DisableThreadLibraryCalls (hinstDLL);

      loop_thread = std::thread (entry);
      loop_thread.detach ();
      break;

    case DLL_PROCESS_DETACH:
      if (lpvReserved != nullptr)
	break; // do not do cleanup if process termination scenario

      if (loop_thread.joinable ())
	loop_thread.join ();
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