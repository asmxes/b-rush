#include <Windows.h>
#include <iostream>

using namespace std;

int
main (int argc, char *argv[], char *envp[])
{
  char *dll_path = argv[1];

  if (GetFileAttributes (dll_path) == INVALID_FILE_ATTRIBUTES)
    {
      cerr << "[ FAILED ] DLL file does not exist." << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  auto hwnd = FindWindow ("VALORANTUnrealWindow",
			  "VALORANT  "); // Game window classname
  if (hwnd == INVALID_HANDLE_VALUE)
    {
      cerr << "[ FAILED ] Could not find target window." << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  DWORD pid = NULL;
  DWORD tid = GetWindowThreadProcessId (hwnd, &pid);
  if (tid == NULL)
    {
      cerr << "[ FAILED ] Could not get thread ID of the target window."
	   << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  auto dll = LoadLibraryEx (dll_path, NULL, DONT_RESOLVE_DLL_REFERENCES);
  if (dll == NULL)
    {
      cerr << "[ FAILED ] The DLL could not be found." << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  auto addr = (HOOKPROC) GetProcAddress (
    dll,
    "dummy_debug_proc"); // export see dllmain.cpp "C" __declspec(dllexport) int
			 // dummy_debug_proc(int code, WPARAM wParam, LPARAM
			 // lParam)
  if (addr == NULL)
    {
      cerr << "[ FAILED ] The function was not found." << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  auto handle = SetWindowsHookEx (WH_GETMESSAGE, addr, dll, tid);
  if (handle == NULL)
    {
      cerr << "[ FAILED ] Couldn't set the hook with SetWindowsHookEx." << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  // Triggering the hook
  PostThreadMessage (tid, WM_NULL, NULL, NULL);
  cout << "[ OK ] Hook set and triggered." << endl;
  Sleep (1500);

  // TODO: Send secret to window, if answers module is loaded

  if (UnhookWindowsHookEx (handle) == FALSE)
    {
      cerr << "[ FAILED ] Could not remove the hook." << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  cout << "[ OK ] Unhooked." << endl;
  Sleep (1500);
  return EXIT_SUCCESS;
}
