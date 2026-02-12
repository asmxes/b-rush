#include "logger.hpp"
#include <Windows.h>

logger *
logger::get ()
{
  static logger obj;
  return &obj;
}

void
logger::configure (const std::string &dir, logger::level lvl)
{
  this->_stream.open (logger::make_filename (dir), std::ios::app);
  this->_lvl = lvl;
}

void
logger::print (logger::level lvl, const char *func, std::string content)
{
  if (lvl < this->_lvl)
    return;

  if (!this->_stream.is_open ())
    {
      MessageBoxA (NULL, "Could not open file", "Chalkboard", MB_OK);
      return;
    }

  this->_stream << "[" << level_char (lvl) << "] [" << func << "] ["
		<< time_stamp () << "] " << content << std::endl;
}

std::string
logger::make_filename (const std::string &dir)
{
  SYSTEMTIME st;
  GetLocalTime (&st);

  char buf[64];
  snprintf (buf, sizeof (buf), "chalkboard-%02d%02d%02d.log", st.wDay,
	    st.wMonth, st.wYear % 100);

  MessageBoxA (NULL, std::string (dir + "\\" + buf).c_str (), "Chalkboard",
	       MB_OK);
  return dir + "\\" + buf;
}

const char *
logger::level_char (logger::level lvl)
{
  switch (lvl)
    {
    case kTRACE:
      return "T";
    case kDEBUG:
      return "D";
    case kINFO:
      return "I";
    case kWARNING:
      return "W";
    case kERROR:
      return "E";
    default:
      return "?";
    }
}
std::string
logger::time_stamp ()
{
  SYSTEMTIME st;
  GetLocalTime (&st);

  char buf[16];
  snprintf (buf, sizeof (buf), "%02d.%02d.%02d.%03d", st.wHour, st.wMinute,
	    st.wSecond, st.wMilliseconds);
  return {buf};
}