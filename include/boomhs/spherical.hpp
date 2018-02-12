#pragma once
#include <stlw/math.hpp>
#include <string>
#include <ostream>

namespace boomhs
{

struct SphericalCoordinates
{
  float radius = 0.0f;
  float theta = 0.0f;
  float phi = 0.0f;

  SphericalCoordinates() = default;
  SphericalCoordinates(float const r, float const t, float const p)
    : radius(r)
    , theta(t)
    , phi(p)
  {
  }
  explicit SphericalCoordinates(glm::vec3 const& v)
    : SphericalCoordinates(v.x, v.y, v.z)
  {
  }

  std::string
  radius_display_string() const { return std::to_string(this->radius); }

  std::string
  theta_display_string() const { return std::to_string(glm::degrees(this->theta)); }

  std::string
  phi_display_string() const { return std::to_string(glm::degrees(this->phi)); }
};

std::ostream&
operator<<(std::ostream &, SphericalCoordinates const&);

glm::vec3
to_cartesian(SphericalCoordinates const&);

SphericalCoordinates
to_spherical(glm::vec3);

} // ns boomhs
