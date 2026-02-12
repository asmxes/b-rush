#include <Windows.h>
#include "overlay/overlay.hpp"
#include "logger/logger.hpp"

#define RY_IMPL
#include "ry/ry.hpp"

BOOL WINAPI
DllMain (HINSTANCE hinstDLL, // handle to DLL module
	 DWORD fdwReason,    // reason for calling function
	 LPVOID lpvReserved) // reserved
{
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

      MessageBoxA (NULL, "Entry", "Chalkboard", MB_OK);
      logger::get ()->configure ("C:\\Users\\ry\\Desktop",
				 logger::level::kTRACE);
      TRACE ("Chalkboard start, main module: "
	     + std::to_string ((u64) GetModuleHandle (nullptr))
	     + ", chalkboard module: " + std::to_string ((u64) hinstDLL));

      overlay::get ()->start ();
      break;

    case DLL_PROCESS_DETACH:

      if (lpvReserved != nullptr)
	break; // do not do cleanup if process termination scenario

      MessageBoxA (NULL, "Exit", "Chalkboard", MB_OK);
      overlay::get ()->stop ();
      break;
    default:
      break;
    }
  return FALSE;
}

extern "C" __declspec (dllexport) int
dummy_debug_proc (int code, WPARAM wParam, LPARAM lParam)
{
  return CallNextHookEx (NULL, code, wParam, lParam);
}