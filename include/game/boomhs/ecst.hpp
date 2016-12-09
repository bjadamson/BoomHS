#pragma once
#include <ecst.hpp>
#include <game/boomhs/ecst_components.hpp>
#include <game/boomhs/ecst_systems.hpp>

namespace ecst_setup
{

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
    .component_signatures(ecst_setup::make_csl())
    .system_signatures(ecst_setup::make_ssl())
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
