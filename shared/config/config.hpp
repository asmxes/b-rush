#ifndef SHARED_CONFIG_HPP
#define SHARED_CONFIG_HPP

#include <string>

#include "defaults.hpp"

namespace config {
std::string
get (const std::string &key, const std::string &default_value);
}

#endif