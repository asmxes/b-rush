#ifndef CHALKBOARD_LOGGER_HPP
#define CHALKBOARD_LOGGER_HPP

#include "ry/ry.hpp"

class logger : public RY_MODULE
{
public:
  enum level
  {
    kTRACE,
    kDEBUG,
    kINFO,
    kWARNING,
    kERROR,
    kMAX_LOGGER_LEVEL
  };

  static logger *get ();
  virtual void configure (const std::string &dir, level lvl);
  virtual void print (level lvl, const char *func, std::string content);
  virtual void update () {};

private:
  static std::string make_filename (const std::string &dir);
  static const char *level_char (level lvl);
  static std::string time_stamp ();

  level _lvl;
  std::ofstream _stream;
};

#define TRACE(x) logger::get ()->print (logger::level::kTRACE, __func__, x)
#define DEBUG(x) logger::get ()->print (logger::level::kDEBUG, __func__, x)
#define INFO(x) logger::get ()->print (logger::level::kINFO, __func__, x)
#define WARNING(x) logger::get ()->print (logger::level::kWARNING, __func__, x)
#define ERROR(x) logger::get ()->print (logger::level::kERROR, __func__, x)

#endif