#pragma once
#include <ecst.hpp>
#include <game/boomhs/ecst_components.hpp>
#include <game/boomhs/io_system.hpp>
#include <game/boomhs/randompos_system.hpp>

namespace st
{
constexpr auto io_system = ecst::tag::system::v<s::io_system>;
constexpr auto randompos_system = ecst::tag::system::v<s::randompos_system>;

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

  constexpr auto PA = ips::none::v();

  // clang-format off
  constexpr auto ssig_io_system = ss::make(st::io_system)
    .parallelism(PA)
    .write(ct::mvmatrix);

  constexpr auto ssig_randompos_system = ss::make(st::randompos_system)
    .parallelism(PA)
    .write(ct::world_coordinate);

  // clang-format on

  // Build and return the "system signature list".
  return sls::make(ssig_io_system, ssig_randompos_system);
}

} // ns ecst_setup
