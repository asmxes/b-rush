#include <Windows.h>
#include <iostream>

#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

using namespace std;

bool
verify_signature (const std::wstring &path,
		  const std::wstring &expected_subject)
{
  // step 1: verify the file is signed and trusted
  WINTRUST_FILE_INFO file_info = {};
  file_info.cbStruct = sizeof (file_info);
  file_info.pcwszFilePath = path.c_str ();

  GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;

  WINTRUST_DATA trust_data = {};
  trust_data.cbStruct = sizeof (trust_data);
  trust_data.dwUIChoice = WTD_UI_NONE;
  trust_data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
  trust_data.dwUnionChoice = WTD_CHOICE_FILE;
  trust_data.pFile = &file_info;
  trust_data.dwStateAction = WTD_STATEACTION_VERIFY;

  LONG status = WinVerifyTrust (NULL, &policy, &trust_data);

  if (status != ERROR_SUCCESS)
    {
      // cleanup
      trust_data.dwStateAction = WTD_STATEACTION_CLOSE;
      WinVerifyTrust (NULL, &policy, &trust_data);
      return false;
    }

  // step 2: extract signer certificate and check subject
  bool match = false;

  CRYPT_PROVIDER_DATA *prov_data
    = WTHelperProvDataFromStateData (trust_data.hWVTStateData);
  if (prov_data)
    {
      CRYPT_PROVIDER_SGNR *signer
	= WTHelperGetProvSignerFromChain (prov_data, 0, FALSE, 0);
      if (signer && signer->pasCertChain && signer->csCertChain > 0)
	{
	  PCCERT_CONTEXT cert = signer->pasCertChain[0].pCert;
	  if (cert)
	    {
	      wchar_t subject[256] = {};
	      CertGetNameStringW (cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL,
				  subject, 256);

	      match = (expected_subject == subject);
	    }
	}
    }

  // cleanup
  trust_data.dwStateAction = WTD_STATEACTION_CLOSE;
  WinVerifyTrust (NULL, &policy, &trust_data);

  return match;
}

bool
verify_self (const std::wstring &expected_subject)
{
  wchar_t self_path[MAX_PATH];
  GetModuleFileNameW (NULL, self_path, MAX_PATH);
  return verify_signature (self_path, expected_subject);
}

int
main (int argc, char *argv[], char *envp[])
{
  auto name = L"Simone Fiorenza";
  if (!verify_self (name))
    return EXIT_FAILURE;

  std::string dll_path{};
  if (argc == 2)
    {
      dll_path = argv[1];
    }
  else if (argc > 2)
    {
      cerr << "[ FAILED ] Args are invalid\nUsage is injector.exe "
	      "OPTIONAL={path to chalkboard.dll}"
	   << endl;
      system ("pause > nul");
      return EXIT_FAILURE;
    }

  if (dll_path.empty ())
    dll_path = "chalkboard.dll";

  if (!verify_signature (std::wstring (dll_path.begin (), dll_path.end ()),
			 name))
    return EXIT_FAILURE;

  if (GetFileAttributes (dll_path.c_str ()) == INVALID_FILE_ATTRIBUTES)
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

  auto dll
    = LoadLibraryEx (dll_path.c_str (), NULL, DONT_RESOLVE_DLL_REFERENCES);
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

  cout << "[ OK ] Done." << endl;
  Sleep (1500);
  return EXIT_SUCCESS;
}
