#pragma once
#include <ecst.hpp>
#include <game/data_types.hpp>

namespace ct
{
// Define component tags.
namespace sc = ecst::signature::component;

constexpr auto world_coordinate = ecst::tag::component::v<game::world_coordinate>;
constexpr auto model = ecst::tag::component::v<game::model>;

} // ns ct

namespace ecst_setup
{

// Builds and returns a "component signature list".
constexpr auto
make_csl()
{
  namespace sc = ecst::signature::component;
  namespace slc = ecst::signature_list::component;

  constexpr auto cs_world_coordinate = sc::make(ct::world_coordinate).contiguous_buffer();
  constexpr auto cs_model = sc::make(ct::model).contiguous_buffer();

  return slc::make(cs_world_coordinate, cs_model);
}

} // ns ecst_setup
