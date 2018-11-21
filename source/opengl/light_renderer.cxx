#include <opengl/light_renderer.hpp>
#include <opengl/renderer.hpp>
#include <opengl/shader.hpp>

#include <boomhs/engine.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/lighting.hpp>
#include <boomhs/material.hpp>
#include <boomhs/transform.hpp>
#include <boomhs/zone_state.hpp>

#include <extlibs/glm.hpp>

#include <vector>

using namespace boomhs;
using namespace opengl;

namespace
{

void
set_dirlight(common::Logger& logger, ShaderProgram& sp, GlobalLight const& global_light)
{
  auto const& directional_light = global_light.directional;
  shader::set_uniform(logger, sp, "u_dirlight.direction", directional_light.direction);

  auto const& light = directional_light.light;
  shader::set_uniform(logger, sp, "u_dirlight.diffuse", light.diffuse);
  shader::set_uniform(logger, sp, "u_dirlight.specular", light.specular);
}

void
set_pointlight(common::Logger& logger, ShaderProgram& sp, size_t const index,
               PointLight const& pointlight, glm::vec3 const& pointlight_position)
{
  std::string const varname = "u_pointlights[" + std::to_string(index) + "]";
  auto const make_field = [&varname](char const* fieldname) { return varname + "." + fieldname; };

  shader::set_uniform(logger, sp, make_field("diffuse"), pointlight.light.diffuse);
  shader::set_uniform(logger, sp, make_field("specular"), pointlight.light.specular);
  shader::set_uniform(logger, sp, make_field("position"), pointlight_position);

  auto const& attenuation       = pointlight.attenuation;
  auto const  attenuation_field = [&make_field](char const* fieldname) {
    return make_field("attenuation.") + fieldname;
  };
  auto const constant  = attenuation_field("constant");
  auto const linear    = attenuation_field("linear");
  auto const quadratic = attenuation_field("quadratic");
  shader::set_uniform(logger, sp, constant, attenuation.constant);
  shader::set_uniform(logger, sp, linear, attenuation.linear);
  shader::set_uniform(logger, sp, quadratic, attenuation.quadratic);
}

struct PointlightTransform
{
  Transform const&  transform;
  PointLight const& pointlight;
};

void
set_receiveslight_uniforms(RenderState& rstate, glm::vec3 const& position,
                           glm::mat4 const& model_matrix, ShaderProgram& sp,
                           Material const&                         material,
                           std::vector<PointlightTransform> const& pointlights,
                           bool const                              set_normalmatrix)
{
  auto&       fstate = rstate.fs;
  auto&       es     = fstate.es;
  auto&       zs     = fstate.zs;
  auto const& ldata  = zs.level_data;

  auto&       logger       = es.logger;
  auto const& global_light = ldata.global_light;
  // auto const& player       = ldata.player;

  render::set_modelmatrix(logger, model_matrix, sp);
  if (set_normalmatrix) {
    glm::mat3 const nmatrix = glm::inverseTranspose(glm::mat3{model_matrix});
    shader::set_uniform(logger, sp, "u_normalmatrix", nmatrix);
  }

  set_dirlight(logger, sp, global_light);

  // ambient
  shader::set_uniform(logger, sp, "u_ambient.color", global_light.ambient);

  // specular
  shader::set_uniform(logger, sp, "u_reflectivity", 1.0f);

  // pointlight
  auto const view_matrix = fstate.view_matrix();
  {
    glm::mat4 const inv_viewmatrix = glm::inverse(glm::mat3{view_matrix});
    shader::set_uniform(logger, sp, "u_invviewmatrix", inv_viewmatrix);
  }

  FOR(i, pointlights.size())
  {
    auto const& transform  = pointlights[i].transform;
    auto const& pointlight = pointlights[i].pointlight;
    set_pointlight(logger, sp, i, pointlight, transform.translation);
  }

  // Material uniforms
  shader::set_uniform(logger, sp, "u_material.ambient", material.ambient);
  shader::set_uniform(logger, sp, "u_material.diffuse", material.diffuse);
  shader::set_uniform(logger, sp, "u_material.specular", material.specular);
  shader::set_uniform(logger, sp, "u_material.shininess", material.shininess);
  // TODO: when re-implementing LOS restrictions
  // shader::set_uniform(logger, sp, "u_player.position",  player.world_position());
  // shader::set_uniform(logger, sp, "u_player.direction",  player.forward_vector());
  // shader::set_uniform(logger, sp, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));
}

} // namespace

namespace opengl
{

void
LightRenderer::set_light_uniforms(RenderState& rstate, EntityRegistry& registry, ShaderProgram& sp,
                                  Material const& material, glm::vec3 const& position,
                                  glm::mat4 const& model_matrix, bool const set_normalmatrix)
{
  auto const                       pointlight_eids = find_pointlights(registry);
  std::vector<PointlightTransform> pointlights;

  FOR(i, pointlight_eids.size())
  {
    auto const&               eid        = pointlight_eids[i];
    auto&                     transform  = registry.get<Transform>(eid);
    auto&                     pointlight = registry.get<PointLight>(eid);
    PointlightTransform const plt{transform, pointlight};

    pointlights.emplace_back(plt);
  }
  set_receiveslight_uniforms(rstate, position, model_matrix, sp, material, pointlights,
                             set_normalmatrix);
}

} // namespace opengl
