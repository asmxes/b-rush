#include "logger.hpp"
#include <Windows.h>
#include <filesystem>

namespace logger {

level _lvl{};
std::ofstream _stream{};
std::string _path{};

std::string
make_filename (const std::string &dir, const std::string &file)
{
  SYSTEMTIME st;
  GetLocalTime (&st);

  char buf[64];
  snprintf (buf, sizeof (buf),
	    std::string (file + "-%02d%02d%02d.log").c_str (), st.wDay,
	    st.wMonth, st.wYear % 100);

  return dir + "\\" + buf;
}

const char *
level_char (logger::level lvl)
{
  switch (lvl)
    {
    case logger::level::kTRACE:
      return "T";
    case logger::level::kDEBUG:
      return "D";
    case logger::level::kINFO:
      return "I";
    case logger::level::kWARNING:
      return "W";
    case logger::level::kERROR:
      return "E";
    default:
      return "?";
    }
}

std::string
time_stamp ()
{
  SYSTEMTIME st;
  GetLocalTime (&st);

  char buf[16];
  snprintf (buf, sizeof (buf), "%02d.%02d.%02d.%03d", st.wHour, st.wMinute,
	    st.wSecond, st.wMilliseconds);
  return {buf};
}

void
print (logger::level lvl, const char *func, std::string content)
{
  if (lvl < _lvl)
    return;

  if (!_stream.is_open ())
    {
#ifdef _DEBUG
      MessageBoxA (NULL,
		   std::format ("Could not open logging file, content: {}",
				content)
		     .c_str (),
		   "Chalkboard", MB_OK | MB_ICONERROR);
#endif
      return;
    }

  _stream << "[" << level_char (lvl) << "] [" << time_stamp () << "] [" << func
	  << "] " << content << std::endl;
}

bool
open (const std::string &dir, const std::string &file, level lvl)
{
  char expanded[MAX_PATH];
  ExpandEnvironmentStringsA (dir.c_str (), expanded, MAX_PATH);

  std::filesystem::create_directories (expanded);

  _path = make_filename (expanded, file);
  _stream.open (_path, std::ios::app);
  _lvl = lvl;

  if (!_stream.is_open ())
    MessageBoxA (NULL,
		 std::format ("Chalkboard failed to save create path for logs "
			      "on: {}.\nLogs will not be saved.",
			      dir)
		   .c_str (),
		 "Chalkboard", MB_OK | MB_ICONERROR);
  return _stream.is_open ();
}

bool
open (const std::string &dir, const std::string &file,
      const std::string &lvl_str)
{
  level lvl{};
  if (lvl_str == "TRACE")
    lvl = logger::level::kTRACE;
  else if (lvl_str == "DEBUG")
    lvl = logger::level::kDEBUG;
  else if (lvl_str == "INFO")
    lvl = logger::level::kINFO;
  else if (lvl_str == "WARNING")
    lvl = logger::level::kWARNING;
  else if (lvl_str == "ERROR")
    lvl = logger::level::kERROR;
  else
    lvl = logger::level::kINFO;

  return open (dir, file, lvl);
}

void
close ()
{
  INFO ("Closing logging output");
  if (_stream.is_open ())
    _stream.close ();
}

const std::string &
get_path ()
{
  return _path;
}
} // namespace logger
