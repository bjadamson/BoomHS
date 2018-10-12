#pragma once
#include <boomhs/audio.hpp>
#include <boomhs/level_manager.hpp>
#include <boomhs/scene_renderer.hpp>
#include <optional>

namespace boomhs
{
struct EngineState;

class GameState
{
  EngineState& es_;
  LevelManager lm_;
  WaterAudioSystem was_;

  std::optional<StaticRenderers> renderers_;
public:
  NOCOPY_MOVE_DEFAULT(GameState);

  explicit GameState(EngineState&, LevelManager&&, WaterAudioSystem&&);

  void set_renderers(StaticRenderers&& sr)
  {
    renderers_ = std::make_optional(MOVE(sr));
  }

  auto& engine_state() { return es_; }
  auto& level_manager() { return lm_; }
  auto& water_audio() { return was_; }

  auto& static_renderers()
  {
    assert(renderers_);
    return *renderers_;
  }
};

} // namespace boomhs
