#include "event.hpp"

namespace utility {
namespace event {

manager *
manager::get ()
{
  static manager object{};
  return &object;
}

void
manager::unsubscribe (id event, ptr owner)
{
  auto found = this->_subs.find (event);
  if (found == this->_subs.end ())
    return;

  auto &subs = found->second;
  subs.erase (std::remove_if (subs.begin (), subs.end (),
			      [owner] (const subscriber &s) {
				return s.owner == owner;
			      }),
	      subs.end ());
}

} // namespace event
} // namespace utility