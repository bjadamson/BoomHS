#include <boomhs/leveldata.hpp>

namespace boomhs
{

LevelData::LevelData(TileGrid &&td, TileSharedInfoTable &&ttable, TilePosition const& start_pos,
    std::vector<RiverInfo> &&rivers, EntityID const torch_eid,

    opengl::Color const& bgcolor, opengl::GlobalLight const& glight,
      ObjCache &&ocache, Camera &&cam, WorldObject &&pl
    )
  : tilegrid_(MOVE(td))
  , ttable_(MOVE(ttable))
  , startpos_(start_pos)
  , rivers_(MOVE(rivers))
  , torch_eid_(torch_eid)
  , background(bgcolor)
  , global_light(glight)
  , obj_cache(MOVE(ocache))
  , camera(MOVE(cam))
  , player(MOVE(pl))
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
