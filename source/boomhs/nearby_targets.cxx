#include <boomhs/nearby_targets.hpp>
#include <boomhs/math.hpp>
#include <window/timer.hpp>

#include <iostream>

using namespace boomhs;
using namespace opengl;
using namespace window;

namespace
{

bool
should_change_offset(std::optional<SelectedTarget> const& selected,
                     std::vector<EntityID> const&         targets)
{
  return selected && (targets.size() > 1);
}

} // namespace

namespace boomhs
{

void
NearbyTargets::add_target(EntityID const target_eid)
{
  targets_.emplace_back(target_eid);
}

float
NearbyTargets::calculate_scale(FrameTime const& ft) const
{
  // GOAL: After time since target last changed, we always return 1. The caller will assume this
  // means full scale, which in this case will be the renderer. The renderer will draw whatever
  // indicates nearby targets at full scale size.
  //
  // Between the time the target is last changed and some other time after then, this function will
  // return a value in the range [0, 1].
  ticks_t constexpr GROW_TIME_MS = 180;

  assert(selected_);
  ticks_t const lc     = millis_when_last_target_changed;
  ticks_t const future = lc + GROW_TIME_MS;
  ticks_t const now    = ft.since_start_millis();

  float constexpr MIN = 0.0f, MAX = 1.0f;
  float const difference = static_cast<float>(future - now);
  float const lerp       = glm::lerp(MIN, MAX, difference);
  float const inverse    = 1 - (lerp / GROW_TIME_MS);
  float const rate       = std::abs(inverse);
  float const clamped    = glm::clamp(rate, MIN, MAX);
  return clamped;
}

void
NearbyTargets::clear()
{
  targets_.clear();

  if (selected_) {
    selected_ = std::nullopt;
  }
}

void
NearbyTargets::cycle(CycleDirection const cycle_dir, FrameTime const& ft)
{
  if (CycleDirection::Forward == cycle_dir) {
    cycle_forward(ft);
  }
  else {
    assert(CycleDirection::Backward == cycle_dir);
    cycle_backward(ft);
  }
}

void
NearbyTargets::cycle_forward(FrameTime const& ft)
{
  if (!selected_ && !targets_.empty()) {
    selected_ = SelectedTarget{};
    update_time(ft);
  }
  else if (should_change_offset(selected_, targets_)) {
    auto& offset = selected_->offset;
    offset += 1;
    if (offset >= targets_.size()) {
      offset = 0;
    }
    update_time(ft);
  }
}

void
NearbyTargets::cycle_backward(FrameTime const& ft)
{
  if (!selected_ && !targets_.empty()) {
    selected_ = SelectedTarget{};
    update_time(ft);
  }
  else if (should_change_offset(selected_, targets_)) {
    auto& offset = selected_->offset;
    if (offset == 0) {
      offset = targets_.size();
    }
    offset -= 1;
    update_time(ft);
  }
}

bool
NearbyTargets::empty() const
{
  return targets_.empty();
}

std::optional<EntityID>
NearbyTargets::selected() const
{
  if (!selected_) {
    return std::nullopt;
  }

  // If an object is selected, then certainly the list of objects the selected object comes from is
  // not empty.
  assert(!empty());

  auto const offset = selected_->offset;
  assert(targets_.size() > offset);
  return targets_[offset];
}

void
NearbyTargets::set_selected(EntityID const selected)
{
  auto const cmp = [&selected](EntityID const eid) { return selected == eid; };
  auto const it  = std::find_if(targets_.cbegin(), targets_.cend(), cmp);
  if (it == targets_.cend()) {
    // entity must no longer be in LOS
    return;
  }
  assert(it != targets_.cend());

  auto const index  = std::distance(targets_.cbegin(), it);
  selected_         = SelectedTarget{};
  selected_->offset = index;
}

void
NearbyTargets::update_time(FrameTime const& ft)
{
  millis_when_last_target_changed = ft.since_start_millis();
}

Color
NearbyTargets::color_from_level_difference(int const player_level, int const target_level)
{
  int const diff = player_level - target_level;
  int const abs_diff = std::abs(diff);

  bool const player_gt = diff > 0;
  bool const player_lt = diff < 0;
  bool const equals = diff == 0;

  {
    bool const equals_and_neither_gtlt     = (equals && (!player_gt && !player_lt));
    bool const gltl_notsame                = ((player_gt && !player_lt)
                                                || (!player_gt && player_lt));
    bool const notequals_and_onlyone_gtlt  = (!equals && gltl_notsame);

    // Ensure that either they are the same (and gt/lt are false)
    // or
    // they are not the same and exactly one (gt/lt) is true.
    assert(equals_and_neither_gtlt || notequals_and_onlyone_gtlt);
  }

  if (equals) {
    return LOC::WHITE;
  }
  else if (player_gt && (abs_diff <= 2)) {
    return LOC::BLUE;
  }
  else if (player_gt && (abs_diff <= 3)) {
    return LOC::LIGHT_BLUE;
  }
  else if (player_gt && (abs_diff <= 3)) {
    return LOC::GREEN;
  }
  else if (player_gt) {
    return LOC::GRAY;
  }
  else if (player_lt && (abs_diff <= 2)) {
    return LOC::YELLOW;
  }
  else if (player_lt && (abs_diff <= 3)) {
    return LOC::ORANGE;
  }
  else if (player_lt) {
    return LOC::RED;
  }
  std::abort();
}

} // namespace boomhs
