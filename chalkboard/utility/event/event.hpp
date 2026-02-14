#ifndef CHALKBOARD_UTILITY_EVENT_HPP
#define CHALKBOARD_UTILITY_EVENT_HPP

#include <unordered_map>
#include <vector>
#include <functional>
#include <any>

#include "typedefs.hpp"
#include "utility/logger/logger.hpp"

namespace utility {

namespace event {

enum id
{
  update,
  render,
  render_menu,
  wnd_proc
};

struct subscriber
{
  ptr owner; // nullptr for free functions, object pointer for methods
  std::function<void (ptr)> fn;
};

class manager
{
  std::unordered_map<id, std::vector<subscriber>> _subs;

  manager () {};

public:
  static manager *get ();

  // free function
  template <typename... Args>
  void subscribe (id event, void (*callback) (Args...))
  {
    this->_subs[event].emplace_back (
      subscriber{nullptr, [callback] (ptr args) {
		   auto &tuple = *static_cast<std::tuple<Args...> *> (args);
		   std::apply (callback, tuple);
		 }});
  }

  // member function
  template <typename T, typename... Args>
  void subscribe (id event, T *object, void (T::*callback) (Args...))
  {
    this->_subs[event].emplace_back (
      subscriber{static_cast<ptr> (object), [object, callback] (ptr args) {
		   auto &tuple = *static_cast<std::tuple<Args...> *> (args);
		   std::apply ([object, callback] (
				 Args... a) { (object->*callback) (a...); },
			       tuple);
		 }});
  }

  void unsubscribe (id event, ptr owner);

  template <typename... Args> void dispatch (id event, Args &&...args)
  {
    auto found = this->_subs.find (event);

    if (found == this->_subs.end ())
      return;

    auto tuple = std::make_tuple (std::forward<Args> (args)...);

    for (auto &sub : found->second)
      sub.fn (static_cast<ptr> (&tuple));
  }
};

} // namespace event
} // namespace utility

#define SUBSCRIBE(...) utility::event::manager::get ()->subscribe (__VA_ARGS__)
#define UNSUBSCRIBE(...)                                                       \
  utility::event::manager::get ()->unsubscribe (__VA_ARGS__)
#define PUBLISH(...) utility::event::manager::get ()->dispatch (__VA_ARGS__)

#endif
