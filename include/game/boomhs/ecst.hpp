#pragma once
#include <ecst.hpp>
#include <game/boomhs/io_system.hpp>

namespace st
{
constexpr auto io_system = ecst::tag::system::v<s::io_system>;

} // ns st

namespace ct
{
// Define component tags.
namespace sc = ecst::signature::component;

constexpr auto world_coordinate = ecst::tag::component::v<game::world_coordinate>;
} // ns ct

// Setup compile-time settings.
namespace ecst_setup
{
// Builds and returns a "component signature list".
constexpr auto
make_csl()
{
  namespace sc = ecst::signature::component;
  namespace slc = ecst::signature_list::component;

  constexpr auto cs_world_coordinate = sc::make(ct::world_coordinate).contiguous_buffer();

  return slc::make(cs_world_coordinate);
}

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

  constexpr auto ssig_io_system = ss::make(st::io_system).parallelism(PA);

  // Build and return the "system signature list".
  return sls::make(ssig_io_system);
}

constexpr auto
make_scheduler()
{
  using namespace ecst_setup;
  namespace cs = ecst::settings;
  namespace ss = ecst::scheduler;

  // Define ECST context settings.
  return ecst::settings::make()
    .allow_inner_parallelism()
    //.disallow_inner_parallelism()
    .fixed_entity_limit(ecst::sz_v<10000>)
    .component_signatures(make_csl())
    .system_signatures(make_ssl())
    .scheduler(cs::scheduler<ss::s_atomic_counter>);
}

auto
make_context()
{
  auto constexpr scheduler = make_scheduler();

  // Create an ECST context.
  return ecst::context::make_uptr(scheduler);
}

} // ns ecst_setup
