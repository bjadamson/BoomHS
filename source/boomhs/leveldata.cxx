#include <boomhs/leveldata.hpp>

namespace boomhs
{

LevelData::LevelData(
                     TerrainGrid&& tgrid, Fog const& fogp,
                     opengl::GlobalLight const& glight, MaterialTable&& mat_table,
                     ObjStore&& ocache)
    : fog(fogp)
    , terrain(MOVE(tgrid))
    , global_light(glight)
    , material_table(MOVE(mat_table))
    , obj_store(MOVE(ocache))
    , time_offset(0.0)
{
}

} // namespace boomhs
