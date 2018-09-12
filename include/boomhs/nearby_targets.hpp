#pragma once
#include <boomhs/colors.hpp>
#include <boomhs/entity.hpp>
#include <optional>
#include <vector>

namespace boomhs
{
class FrameTime;

struct SelectedTarget
{
  // Offset into the array of target entity id's that this selected target represents.
  size_t offset = 0;
};

enum class CycleDirection
{
  Forward = 0,
  Backward
};

class NearbyTargets
{
  std::vector<EntityID> targets_ = {};

  uint64_t                      millis_when_last_target_changed = 0;
  std::optional<SelectedTarget> selected_                       = std::nullopt;

  bool empty() const;
  void update_time(FrameTime const&);

public:
  NearbyTargets() = default;

  float calculate_scale(FrameTime const&) const;

  void add_target(EntityID);
  void clear();
  void cycle(CycleDirection, FrameTime const&);
  void cycle_forward(FrameTime const&);
  void cycle_backward(FrameTime const&);

  std::optional<EntityID> selected() const;
  void                    set_selected(EntityID);

  static Color color_from_level_difference(int, int);
};

} // namespace boomhs
