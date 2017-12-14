#pragma once
#include <ecst.hpp>
#include <boomhs/ecst_components.hpp>
#include <boomhs/player_system.hpp>
#include <boomhs/randompos_system.hpp>

namespace st
{
constexpr auto randompos_system = ecst::tag::system::v<s::randompos_system>;
constexpr auto player_system = ecst::tag::system::v<s::player_system>;

} // ns st

namespace ecst_setup
{

// Builds and returns a "system signature list".
constexpr auto
make_ssl()
{
  // Signature namespace aliases.
  namespace ss = ecst::signature::system;
  namespace sls = ecst::signature_list::system;

  // Inner parallelism aliases and definitions.
  namespace ips = ecst::inner_parallelism::strategy;
  namespace ipc = ecst::inner_parallelism::composer;

  //constexpr auto PA = ips::none::v();
  constexpr auto PA = ips::split_evenly_fn::v_cores();

  // clang-format off
  constexpr auto ssig_randompos_system = ss::make(st::randompos_system)
    .parallelism(PA);

  constexpr auto ssig_player_system = ss::make(st::player_system)
    .parallelism(PA)
    .write(ct::model);

  // clang-format on

  // Build and return the "system signature list".
  return sls::make(ssig_randompos_system, ssig_player_system);
}

} // ns ecst_setup
