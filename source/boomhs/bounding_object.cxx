#include <boomhs/bounding_object.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/leveldata.hpp>
#include <boomhs/math.hpp>

#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>

using namespace opengl;

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// AABoundingBox
AABoundingBox::AABoundingBox(glm::vec3 const& minp, glm::vec3 const& maxp,
                             DrawInfo&& dinfo)
    : cube(Cube{minp, maxp})
    , draw_info(MOVE(dinfo))
{
}

AABoundingBox&
AABoundingBox::add_to_entity(common::Logger& logger, ShaderPrograms& sps, EntityID const eid,
                             EntityRegistry& registry, glm::vec3 const& min, glm::vec3 const& max)
{
  auto& va    = sps.ref_sp("wireframe").va();

  auto const cv = OF::cube_vertices(min, max);
  auto dinfo = opengl::gpu::copy_cube_wireframe_gpu(logger, cv, va);
  auto& bbox = registry.assign<AABoundingBox>(eid, min, max, MOVE(dinfo));

  registry.assign<Selectable>(eid);
  return bbox;
}

} // namespace boomhs