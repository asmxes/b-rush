#ifndef CHALKBOARD_OVERLAY_HPP
#define CHALKBOARD_OVERLAY_HPP

#include "ry/ry.hpp"

class overlay : public RY_MODULE
{
  virtual std::string get_name ();
  virtual void update ();

public:
  static overlay *get ();
  virtual void start ();
  virtual void stop ();
};
#endif