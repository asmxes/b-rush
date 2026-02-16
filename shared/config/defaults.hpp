#ifndef SHARED_CONFIG_DEFAULTS_HPP
#define SHARED_CONFIG_DEFAULTS_HPP

#define AUTO constexpr auto

namespace config {
namespace defaults {

AUTO kLOG_LEVEL = "INFO";
AUTO kWS_URI = "wss://ws.b-rush.it"; // [ ! ] 'WS(S)://' MUST BE INCLUDED SO THE
				     // CLIENT KNOWS IF A SECURE CONNECTION IS
				     // NEEDED -> DONT CHANGE <-
AUTO kROOM_OVERRIDE = "";

} // namespace defaults
} // namespace config

#undef AUTO

#endif
