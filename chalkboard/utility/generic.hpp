#ifndef CHALKBOARD_UTILITY_GENERIC_HPP
#define CHALKBOARD_UTILITY_GENERIC_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace utility {

template <typename T>
T
get_nested_value (const nlohmann::json &container, const std::string &key)
{
  try
    {
      return container.at (key).get<T> ();
    }
  catch (...)
    {
      return T{};
    }
}

template <typename T, typename... Keys>
T
get_nested_value (const nlohmann::json &container, const std::string &key,
		  const Keys &...keys)
{
  try
    {
      return get_nested_value<T> (container.at (key), keys...);
    }
  catch (...)
    {
      return T{};
    }
}

std::string
read_file (const std::string &path);

std::vector<std::string>
split_string (const std::string &s, const char delim);

std::string
base64_encode (const std::string &decoded_string);

std::string
url_encode (const std::string &input);
} // namespace utility

#endif
