#include "logger.hpp"
#include <Windows.h>

logger::~logger () { this->close (); }

void
logger::print (logger::level lvl, const char *func, std::string content)
{
  if (lvl < this->_lvl)
    return;

  if (!this->_stream.is_open ())
    {
      MessageBoxA (NULL, "Could not open logging file", "Chalkboard",
		   MB_OK | MB_ICONERROR);
      return;
    }

  this->_stream << "[" << level_char (lvl) << "] [" << time_stamp () << "] ["
		<< func << "] " << content << std::endl;
}

void
logger::open (const std::string &dir, level lvl)
{
  this->_path = make_filename (dir);
  this->_stream.open (this->_path, std::ios::app);
  this->_lvl = lvl;
}
void
logger::close ()
{
  INFO ("Closing logging output");
  if (this->_stream.is_open ())
    this->_stream.close ();
}

const std::string &
logger::get_path () const
{
  return this->_path;
}

logger *
logger::get ()
{
  static logger object{};
  return &object;
}

std::string
logger::make_filename (const std::string &dir)
{
  SYSTEMTIME st;
  GetLocalTime (&st);

  char buf[64];
  snprintf (buf, sizeof (buf), "chalkboard-%02d%02d%02d.log", st.wDay,
	    st.wMonth, st.wYear % 100);

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