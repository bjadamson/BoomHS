#pragma once
#include <ecst.hpp>

// TODO: uncouple ecst from rendering engine?
#include <boomhs/types.hpp>

namespace ct
{
// Define component tags.
namespace sc = ecst::signature::component;

constexpr auto transform = ecst::tag::component::v<::boomhs::Transform>;

} // ns ct

namespace ecst_setup
{

// Builds and returns a "component signature list".
constexpr auto
make_csl()
{
  namespace sc = ecst::signature::component;
  namespace slc = ecst::signature_list::component;

  constexpr auto cs_transform = sc::make(ct::transform).contiguous_buffer();

  return slc::make(cs_transform);
}

} // ns ecst_setup
