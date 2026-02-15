#ifndef CHALKBOARD_API_RIOT_CLIENT_HPP
#define CHALKBOARD_API_RIOT_CLIENT_HPP

#include "cpr/cprtypes.h"

#include <string>

#include <nlohmann/json.hpp>

namespace api {
namespace riot {
class client
{
  std::string _lockfile_path, _local_url, _local_auth, _glz_url, _platform_type,
    _current_version, _region, _access_token, _token, _subject;
  bool _is_valid;

  client ();
  std::string get_region ();
  std::string get_current_valorant_version ();
  bool save_online_tokens ();
  bool make_online_header (cpr::Header *header);
  nlohmann::json get_match_data ();

public:
  std::string get_player_guid ();
  std::string get_riot_id ();
  std::string get_match_guid ();
  std::string get_team ();
  static client *get ();
};
} // namespace riot
} // namespace api

#endif