#pragma once
#include <tuple>

namespace game
{

class world_coordinate
{
  float x_ = 0.f, y_ = 0.f, z_ = 0.f, w_ = 0.f;

public:
  constexpr world_coordinate() = default;
  constexpr world_coordinate(float const xp, float const yp, float const zp, float const wp)
    : x_(xp)
    , y_(yp)
    , z_(zp)
    , w_(wp)
  {
  }

#define DEFINE_GET_AND_SET_METHODS(a) \
  constexpr float a() const { return this->a##_; } \
  void set_##a(float const v) { this->a##_ = v; }

  DEFINE_GET_AND_SET_METHODS(x)
  DEFINE_GET_AND_SET_METHODS(y)
  DEFINE_GET_AND_SET_METHODS(z)
  DEFINE_GET_AND_SET_METHODS(w)

  void
  set(float const xp, float const yp, float const zp, float const wp)
  {
    this->set_x(xp);
    this->set_y(yp);
    this->set_z(zp);
    this->set_w(wp);
  }

  std::tuple<float, float, float, float>
  get() const
  {
    return {this->x(), this->y(), this->z(), this->w()};
  }
};

} // ns game
