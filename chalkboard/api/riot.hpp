#ifndef CHALKBOARD_API_RIOT_HPP
#define CHALKBOARD_API_RIOT_HPP

#include "cpr/cprtypes.h"

#include <string>

#include <nlohmann/json.hpp>

namespace api {
namespace riot {
std::string
get_player_guid ();

std::string
get_riot_id ();

std::string
get_match_guid ();

std::string
get_team ();

bool
connect ();
} // namespace riot
} // namespace api

#endif