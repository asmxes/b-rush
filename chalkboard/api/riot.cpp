#include "riot.hpp"

#include "overlay/draw/draw.hpp"
#include "utility/logger/logger.hpp"

#include <format>
#include <filesystem>
#include <cpr/cpr.h>

#include "utility/generic.hpp"

namespace api {
namespace riot {

std::string _lockfile_path{};
std::string _local_url{};
std::string _local_auth{};
std::string _glz_url{};
std::string _platform_type{};
std::string _current_version{};
std::string _region{};
std::string _access_token{};
std::string _token{};
std::string _subject{};
bool _is_valid{};

std::string
get_current_valorant_version ()
{
  auto req = Get (cpr::Url{"https://valorant-api.com/v1/version"},
		  cpr::VerifySsl{false});

  if (req.error.code != cpr::ErrorCode::OK
      || !nlohmann::json::accept (req.text))
    {
      ERROR ("Get on '{}' returned: {}, with message: {}, content: {}",
	     "https://valorant-api.com/v1/version",
	     static_cast<int> (req.error.code), req.error.message, req.text);
      return "";
    }

  auto parsed_json = nlohmann::json::parse (req.text);

  // Could also get directly the riotClientVersion key
  return std::format (
    "{}-shipping-{}-{}",
    parsed_json.at ("data").at ("branch").get<std::string> (),
    parsed_json.at ("data").at ("buildVersion").get<std::string> (),
    utility::split_string (
      parsed_json.at ("data").at ("version").get<std::string> (), '.')
      .at (3));
}

std::string
get_region ()
{
  const auto req
    = Get (cpr::Url{std::string (_local_url)
		      .append (R"(/product-session/v1/external-sessions)")},
	   cpr::Header{{"Authorization", _local_auth}}, cpr::VerifySsl{false});

  if (req.error.code != cpr::ErrorCode::OK
      || !nlohmann::json::accept (req.text))
    {
      {
	ERROR ("Get on '{}' returned: {}, with message: {}, content: {}",
	       "/product-session/v1/external-sessions",
	       static_cast<int> (req.error.code), req.error.message, req.text);
	return "";
      }
    }

  auto sessions = nlohmann::json::parse (req.text);
  for (auto &[key, session] : sessions.items ())
    {
      if (session.value ("productId", "") != "valorant")
	continue;

      for (auto &arg : session["launchConfiguration"]["arguments"])
	{
	  auto s = arg.get<std::string> ();
	  const std::string prefix = "-ares-deployment=";
	  if (s.starts_with (prefix))
	    {
	      return s.substr (prefix.size ());
	      break;
	    }
	}
      break;
    }

  WARNING ("No suitable key found");
  return "";
}

bool
save_online_tokens ()
{
  if (_local_url.empty () || _local_auth.empty ())
    {
      ERROR ("Client is not ready");
      return false;
    }

  auto url = std::string (_local_url).append (R"(/entitlements/v1/token)");
  const auto req
    = Get (cpr::Url{url}, cpr::Header{{"Authorization", _local_auth}},
	   cpr::VerifySsl{false});

  if (req.error.code != cpr::ErrorCode::OK
      || !nlohmann::json::accept (req.text))
    {
      ERROR ("Get on '{}' returned: {}, with message: {}, content: {}", url,
	     static_cast<int> (req.error.code), req.error.message, req.text);
      return false;
    }

  const auto parsed_json = nlohmann::json::parse (req.text);
  if (!parsed_json.contains ("accessToken") || !parsed_json.contains ("token")
      || !parsed_json.contains ("subject"))
    {
      ERROR ("Could not find accessToken, token, subject keys: {}", req.text);
      return false;
    }

  _access_token = parsed_json.at ("accessToken").get<std::string> ();
  _token = parsed_json.at ("token").get<std::string> ();
  _subject = parsed_json.at ("subject").get<std::string> ();
  return true;
}

bool
make_online_header (cpr::Header *header)
{
  if (!header || !save_online_tokens () || _current_version.empty ()
      || _platform_type.empty ())
    {
      ERROR ("Client is not ready");
      return false;
    }

  *header = cpr::Header{
    {"Authorization", std::string ("Bearer ").append (_access_token)},
    {"X-Riot-Entitlements-JWT", _token},
    {"X-Riot-ClientPlatform", _platform_type},
    {"X-Riot-ClientVersion", _current_version},
  };
  return true;
}

std::string
get_player_guid ()
{
  return save_online_tokens () ? _subject : "";
}

std::string
get_riot_id ()
{
  if (!_is_valid)
    return "";

  auto url
    = std::string (_local_url).append (R"(/riot-client-auth/v1/userinfo)");
  auto req = Get (cpr::Url{url}, cpr::Header{{"Authorization", _local_auth}},
		  cpr::VerifySsl{false});

  if (req.error.code != cpr::ErrorCode::OK
      || !nlohmann::json::accept (req.text))
    {
      ERROR ("Get on '{}' returned: {}, with message: {}, content: {}", url,
	     static_cast<int> (req.error.code), req.error.message, req.text);
      return "";
    }

  const auto parsed_json = nlohmann::json::parse (req.text);
  if (!parsed_json.contains ("acct"))
    {
      ERROR ("Could not find acct key: {}", req.text);
      return "";
    }

  auto username = parsed_json.at ("acct").at ("game_name").get<std::string> ();
  auto tag = parsed_json.at ("acct").at ("tag_line").get<std::string> ();
  return username + "#" + tag;
}

nlohmann::json
get_match_data ()
{
  if (!_is_valid)
    {
      ERROR ("Not connected");
      return {};
    }

  cpr::Header header;
  if (!make_online_header (&header))
    {
      ERROR ("Invalid header");
      return {};
    }

  auto url = std::string (_glz_url).append (
    std::format (R"(/core-game/v1/players/{})", _subject));
  const auto req = Get (cpr::Url{url}, header, cpr::VerifySsl{false});

  if (req.error.code != cpr::ErrorCode::OK
      || !nlohmann::json::accept (req.text))
    {
      ERROR ("Get on '{}' returned: {}, with message: {}, content: {}", url,
	     static_cast<int> (req.error.code), req.error.message, req.text);
      return {};
    }

  const auto parsed_json = nlohmann::json::parse (req.text);
  if (!parsed_json.contains ("MatchID"))
    {
      TRACE ("Could not find MatchID key: {}", req.text);
      return {};
    }

  const auto match_id = parsed_json.at ("MatchID").get<std::string> ();
  url = std::string (_glz_url).append (
    std::format (R"(/core-game/v1/matches/{})", match_id));
  const auto match_req = Get (cpr::Url{url}, header, cpr::VerifySsl{false});

  if (match_req.error.code != cpr::ErrorCode::OK
      || !nlohmann::json::accept (match_req.text))
    {
      WARNING ("Get on '{}' returned: {}, with message: {}, content: {}", url,
	       static_cast<int> (match_req.error.code), match_req.error.message,
	       match_req.text);
      return {};
    }

  auto match_json = nlohmann::json::parse (match_req.text);
  return match_json;
}

std::string
get_match_guid ()
{
  auto match_data = get_match_data ();
  if (match_data.empty ())
    {
      DEBUG ("No match data");
      return "";
    }

  if (!match_data.contains ("MatchID") || !match_data.contains ("State"))
    {
      DEBUG ("Could not find MatchID or State keys: {}", match_data.dump ());
      return "";
    }

  auto state = match_data.at ("State").get<std::string> ();
  if (state != "IN_PROGRESS")
    return "";

  auto id = match_data.at ("MatchID").get<std::string> ();
  return id;
}

std::string
get_team ()
{
  auto match_data = get_match_data ();
  if (match_data.empty ())
    {
      DEBUG ("No match data");
      return "";
    }

  std::string team_id{};
  if (match_data.contains ("Players") && match_data["Players"].is_array ())
    {
      for (const auto &player : match_data["Players"])
	{
	  if (player.contains ("Subject") && player.contains ("TeamID"))
	    {
	      if (player["Subject"].get<std::string> () == _subject)
		{
		  team_id = player["TeamID"].get<std::string> ();
		  break;
		}
	    }
	  else
	    {
	      DEBUG ("Could not find Subject, TeamID keys: {}",
		     match_data.dump ());
	    }
	}
    }
  else
    {
      {
	DEBUG ("Could not find Player array: {}", match_data.dump ());
      }
    }

  return team_id;
}

bool
connect ()
{
  _lockfile_path = std::format (R"({}\Riot Games\Riot Client\Config\lockfile)",
				getenv ("LOCALAPPDATA"));
  if (!std::filesystem::exists (_lockfile_path))
    {
      ERROR ("No lockfile was found");
      return false;
    }

  auto file = utility::read_file (_lockfile_path);
  if (file.empty ())
    {
      ERROR ("Lockfile is empty");
      return false;
    }

  auto splitted_file = utility::split_string (file, ':');
  if (splitted_file.size () < 5)
    {
      ERROR ("Lockfile is malformed, content is: {}", file);
      return false;
    }

  _local_url = std::format (R"(https://127.0.0.1:{})", splitted_file.at (2));
  _local_auth = std::string ("Basic ").append (
    utility::base64_encode (std::format ("riot:{}", splitted_file.at (3))));

  _region = get_region ();
  if (_region.empty ())
    {
      ERROR ("No region found");
      return false;
    }
  _glz_url = std::format (R"(https://glz-{}-1.{}.a.pvp.net)", _region, _region);

  nlohmann::json platform;
  platform["platformType"] = "PC";
  platform["platformOS"] = "Windows";
  platform["platformOSVersion"] = "";
  platform["platformChipset"] = "Unknown";
  _platform_type = utility::base64_encode (platform.dump ());

  _current_version = get_current_valorant_version ();
  _is_valid = !_current_version.empty ();
  return true;
}

} // namespace riot
} // namespace api