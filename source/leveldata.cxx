#include <boomhs/leveldata.hpp>

namespace boomhs
{

LevelData::LevelData(TileGrid&& td, TileSharedInfoTable&& ttable, TilePosition const& start_pos,
                     std::vector<RiverInfo>&& rivers, TerrainGrid&& tgrid, Fog const& fogp,
                     opengl::GlobalLight const& glight, ObjStore&& ocache, WorldObject&& pl)
    : tilegrid_(MOVE(td))
    , ttable_(MOVE(ttable))
    , startpos_(start_pos)
    , rivers_(MOVE(rivers))
    , fog(fogp)
    , terrain(MOVE(tgrid))
    , global_light(glight)
    , obj_store(MOVE(ocache))
    , player(MOVE(pl))
    , wind_speed(50.0f)
    , wave_strength(0.01f)
    , time_offset(0.0)
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

} // namespace boomhs
