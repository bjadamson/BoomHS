#pragma once
#include <boomhs/entity.hpp>
#include <optional>
#include <vector>

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{

struct SelectedTarget
{
  // Offset into the array of target entity id's that this selected target represents.
  size_t offset = 0;
};

class NearbyTargets
{
  std::vector<EntityID>         targets_   = {};

  uint64_t millis_when_last_target_changed = 0;
  std::optional<SelectedTarget> selected_  = std::nullopt;

  bool empty() const;
  void update_time(window::FrameTime const&);

public:
  NearbyTargets() = default;

  float calculate_scale(window::FrameTime const&) const;

  void add_target(EntityID);
  void clear();
  void cycle_forward(window::FrameTime const&);
  void cycle_backward(window::FrameTime const&);

  std::optional<EntityID> selected() const;
  void set_selected(EntityID);
};

} // namespace boomhs
