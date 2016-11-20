#pragma once
#include <tuple>

namespace game
{

struct vertex
{
  float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
  vertex() = default;
  vertex(float const xp, float const yp, float const zp, float const wp)
    : x(xp)
    , y(yp)
    , z(zp)
    , w(wp)
    {}
};

class world_coordinate
{
  vertex v_ = {};

public:
  world_coordinate() = default;
  explicit world_coordinate(float const x, float const y, float const z, float const w)
    : v_(x, y, z, w)
  {
  }

#define DEFINE_GET_AND_SET_METHODS(A)                     \
  float A() const { return this->v_.A; }                  \
  void set_##A(float const value) { this->v_.A = value; }

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

  vertex
  get() const
  {
    return this->v_;
  }
};

struct triangle {};
struct rectangle {
};

template<typename T>
class shape {
  world_coordinate coord_;
public:
  explicit shape(class world_coordinate const wc) :
    coord_(wc)
  {}
  auto const& wc() const { return this->coord_; }
};

} // ns game
