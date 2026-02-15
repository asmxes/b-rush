#include "generic.hpp"

#include <filesystem>
#include <fstream>

namespace utility {
std::string
read_file (const std::string &path)
{
  if (!std::filesystem::exists (path))
    return {};

  std::ifstream file (path.data (), std::ios::binary);
  if (!file.good ())
    return {};

  file.unsetf (std::ios::skipws);

  file.seekg (0, std::ios::end);
  const size_t size = file.tellg ();
  file.seekg (0, std::ios::beg);

  std::vector<char> out;

  out.resize (size);
  file.read (out.data (), size);
  file.close ();

  return std::string (out.begin (), out.end ());
}

std::vector<std::string>
split_string (const std::string &s, const char delim)
{
  std::stringstream ss (s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline (ss, item, delim))
    {
      elems.push_back (std::move (item));
    }

  return elems;
}

std::string
base64_encode (const std::string &decoded_string)
{
  const auto base64_chars = std::string (
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

  std::string out;

  int val = 0, valb = -6;
  for (const unsigned char c : decoded_string)
    {
      val = (val << 8) + c;
      valb += 8;
      while (valb >= 0)
	{
	  out.push_back (base64_chars[val >> valb & 0x3F]);
	  valb -= 6;
	}
    }
  if (valb > -6)
    out.push_back (base64_chars[val << 8 >> valb + 8 & 0x3F]);
  while (out.size () % 4)
    out.push_back ('=');

  return out;
}

std::string
url_encode (const std::string &input)
{
  std::string output;
  output.reserve (input.size ());

  for (const char c : input)
    {
      if (std::isalnum (c) || c == '-' || c == '_' || c == '.' || c == '~')
	{
	  output += c;
	}
      else
	{
	  output += '%';
	  output += "0123456789ABCDEF"[c >> 4];
	  output += "0123456789ABCDEF"[c & 15];
	}
    }

  return output;
}

} // namespace utility