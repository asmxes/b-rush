#ifndef SHARED_LOGGER_HPP
#define SHARED_LOGGER_HPP

#include <string>
#include <fstream>
#include <format>

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
open (const std::string &dir, const std::string &file, level lvl);

bool
open (const std::string &dir, const std::string &file,
      const std::string &lvl_str);

void
close ();

const std::string &
get_path ();
} // namespace logger

#define TRACE(...)                                                             \
  logger::print (logger::level::kTRACE, __FUNCTION__, std::format (__VA_ARGS__))
#define DEBUG(...)                                                             \
  logger::print (logger::level::kDEBUG, __FUNCTION__, std::format (__VA_ARGS__))
#define INFO(...)                                                              \
  logger::print (logger::level::kINFO, __FUNCTION__, std::format (__VA_ARGS__))
#define WARNING(...)                                                           \
  logger::print (logger::level::kWARNING, __FUNCTION__,                        \
		 std::format (__VA_ARGS__))
#define ERROR(...)                                                             \
  logger::print (logger::level::kERROR, __FUNCTION__, std::format (__VA_ARGS__))

#endif