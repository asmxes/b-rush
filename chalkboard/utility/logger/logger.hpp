#ifndef CHALKBOARD_LOGGER_HPP
#define CHALKBOARD_LOGGER_HPP

#include <string>
#include <fstream>
#include <format>

namespace utility {
namespace logger {
enum level
{
  kTRACE,
  kDEBUG,
  kINFO,
  kWARNING,
  kERROR,
  kMAX_LOGGER_LEVEL
};

void
print (level lvl, const char *func, std::string content);

bool
open (const std::string &dir, level lvl);

void
close ();

const std::string &
get_path ();
} // namespace logger
} // namespace utility

#define TRACE(...)                                                             \
  utility::logger::print (utility::logger::level::kTRACE, __FUNCTION__,        \
			  std::format (__VA_ARGS__))
#define DEBUG(...)                                                             \
  utility::logger::print (utility::logger::level::kDEBUG, __FUNCTION__,        \
			  std::format (__VA_ARGS__))
#define INFO(...)                                                              \
  utility::logger::print (utility::logger::level::kINFO, __FUNCTION__,         \
			  std::format (__VA_ARGS__))
#define WARNING(...)                                                           \
  utility::logger::print (utility::logger::level::kWARNING, __FUNCTION__,      \
			  std::format (__VA_ARGS__))
#define ERROR(...)                                                             \
  utility::logger::print (utility::logger::level::kERROR, __FUNCTION__,        \
			  std::format (__VA_ARGS__))

#endif