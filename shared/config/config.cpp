#include "config.hpp"
#include <unordered_map>
#include <fstream>
#include <windows.h>

namespace config {

bool _loaded{false};
bool _load_try{false};
std::unordered_map<std::string, std::string> _entries{};

bool
try_load (const std::string &path)
{
  std::ifstream file (path);
  if (!file.is_open ())
    return false;

  std::string line;
  while (std::getline (file, line))
    {
      // skip empty lines and comments
      if (line.empty () || line[0] == '#' || line[0] == ';')
	continue;

      auto eq = line.find ('=');
      if (eq == std::string::npos)
	continue;

      std::string key = line.substr (0, eq);
      std::string value = line.substr (eq + 1);

      // trim whitespace
      auto trim = [] (std::string &s) {
	while (!s.empty () && s.front () == ' ')
	  s.erase (s.begin ());
	while (!s.empty () && s.back () == ' ')
	  s.pop_back ();
      };

      trim (key);
      trim (value);

      if (!key.empty ())
	_entries[key] = value;
    }

  return !_entries.empty ();
}

inline void
load ()
{
  if (_loaded)
    return;

  _load_try = true;

  char expanded[MAX_PATH];
  ExpandEnvironmentStringsA ("%APPDATA%\\b-rush\\.config", expanded, MAX_PATH);
  _loaded = try_load (expanded);
}

std::string
get (const std::string &key, const std::string &default_value)
{
  if (!_load_try)
    load ();

  if (!_loaded && _load_try)
    return default_value;

  auto it = _entries.find (key);
  if (it != _entries.end ())
    return it->second;
  return default_value;
}
} // namespace config