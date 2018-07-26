#include <boomhs/leveldata.hpp>

namespace boomhs
{

LevelData::LevelData(
                     TerrainGrid&& tgrid, Fog const& fogp,
                     opengl::GlobalLight const& glight, ObjStore&& ocache, WorldObject&& pl)
    : fog(fogp)
    , terrain(MOVE(tgrid))
    , global_light(glight)
    , obj_store(MOVE(ocache))
    , player(MOVE(pl))
    , time_offset(0.0)
{
}

} // namespace boomhs
