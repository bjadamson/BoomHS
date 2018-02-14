#include <boomhs/leveldata.hpp>

namespace boomhs
{

LevelData::LevelData(TileData &&td, TileInfos &&tinfos, TilePosition const& start_pos,
    std::vector<RiverInfo> &&rivers)
  : tilegrid_(MOVE(td))
  , tile_infos_(MOVE(tinfos))
  , startpos_(start_pos)
  , rivers_(MOVE(rivers))
{
}

void
LevelData::set_tile(TilePosition const& tpos, TileType const& type)
{
  tilegrid_.data(tpos).type = type;
}

void
LevelData::set_floor(TilePosition const& tpos)
{
  set_tile(tpos, TileType::FLOOR);
}

void
LevelData::set_river(TilePosition const& tpos)
{
  set_tile(tpos, TileType::RIVER);
}

void
LevelData::set_wall(TilePosition const& tpos)
{
  set_tile(tpos, TileType::WALL);
}

bool
LevelData::is_tile(TilePosition const& tpos, TileType const& type) const
{
  return tilegrid_.data(tpos).type == type;
}

bool
LevelData::is_wall(TilePosition const& tpos) const
{
  return is_tile(tpos, TileType::WALL);
}

} // ns boomhs
