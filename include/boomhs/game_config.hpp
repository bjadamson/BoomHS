#pragma once

namespace boomhs
{

enum class GameGraphicsMode
{
  Basic = 0,
  Medium,
  Advanced
};

struct GameGraphicsSettings
{
  GameGraphicsMode mode              = GameGraphicsMode::Basic;
  bool             disable_sunshafts = true;
};

} // namespace boomhs
