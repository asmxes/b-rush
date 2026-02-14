#ifndef CHALKBOARD_LOGGER_HPP
#define CHALKBOARD_LOGGER_HPP

#include <string>
#include <fstream>
#include <format>

class logger
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

  virtual void print (level lvl, const char *func, std::string content);
  virtual void open (const std::string &dir, level lvl);
  virtual void close ();
  virtual const std::string &get_path () const;
  static logger *get ();

private:
  logger () {};
  ~logger ();

  static std::string make_filename (const std::string &dir);
  static const char *level_char (level lvl);
  static std::string time_stamp ();

  level _lvl;
  std::ofstream _stream;
  std::string _path;
};

#define TRACE(...)                                                             \
  logger::get ()->print (logger::level::kTRACE, __FUNCTION__,                  \
			 std::format (__VA_ARGS__))
#define DEBUG(...)                                                             \
  logger::get ()->print (logger::level::kDEBUG, __FUNCTION__,                  \
			 std::format (__VA_ARGS__))
#define INFO(...)                                                              \
  logger::get ()->print (logger::level::kINFO, __FUNCTION__,                   \
			 std::format (__VA_ARGS__))
#define WARNING(...)                                                           \
  logger::get ()->print (logger::level::kWARNING, __FUNCTION__,                \
			 std::format (__VA_ARGS__))
#define ERROR(...)                                                             \
  logger::get ()->print (logger::level::kERROR, __FUNCTION__,                  \
			 std::format (__VA_ARGS__))

#endif