#pragma once
#include <ecst.hpp>

// TODO: uncouple ecst from rendering engine?
#include <gfx/types.hpp>

namespace ct
{
// Define component tags.
namespace sc = ecst::signature::component;

constexpr auto model = ecst::tag::component::v<::gfx::model>;

} // ns ct

namespace ecst_setup
{

// Builds and returns a "component signature list".
constexpr auto
make_csl()
{
  namespace sc = ecst::signature::component;
  namespace slc = ecst::signature_list::component;

  constexpr auto cs_model = sc::make(ct::model).contiguous_buffer();

  return slc::make(cs_model);
}

} // ns ecst_setup
