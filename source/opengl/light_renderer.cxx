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
  sp.set_uniform_vec3(logger, "u_dirlight.direction", directional_light.direction);

  auto const& light = directional_light.light;
  sp.set_uniform_color_3fv(logger, "u_dirlight.diffuse", light.diffuse);
  sp.set_uniform_color_3fv(logger, "u_dirlight.specular", light.specular);
}

void
set_pointlight(common::Logger& logger, ShaderProgram& sp, size_t const index,
               PointLight const& pointlight, glm::vec3 const& pointlight_position)
{
  std::string const varname = "u_pointlights[" + std::to_string(index) + "]";
  auto const make_field = [&varname](char const* fieldname) { return varname + "." + fieldname; };

  sp.set_uniform_color_3fv(logger, make_field("diffuse"), pointlight.light.diffuse);
  sp.set_uniform_color_3fv(logger, make_field("specular"), pointlight.light.specular);
  sp.set_uniform_vec3(logger, make_field("position"), pointlight_position);

  auto const& attenuation       = pointlight.attenuation;
  auto const  attenuation_field = [&make_field](char const* fieldname) {
    return make_field("attenuation.") + fieldname;
  };
  auto const constant  = attenuation_field("constant");
  auto const linear    = attenuation_field("linear");
  auto const quadratic = attenuation_field("quadratic");
  sp.set_uniform_float1(logger, constant, attenuation.constant);
  sp.set_uniform_float1(logger, linear, attenuation.linear);
  sp.set_uniform_float1(logger, quadratic, attenuation.quadratic);
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
  //auto const& player       = ldata.player;

  render::set_modelmatrix(logger, model_matrix, sp);
  if (set_normalmatrix) {
    sp.set_uniform_matrix_3fv(logger, "u_normalmatrix",
                              glm::inverseTranspose(glm::mat3{model_matrix}));
  }

  set_dirlight(logger, sp, global_light);

  // ambient
  LOG_TRACE_SPRINTF("AMBIENT COLOR: %s", global_light.ambient.to_string());
  sp.set_uniform_color_3fv(logger, "u_ambient.color", global_light.ambient);

  // specular
  sp.set_uniform_float1(logger, "u_reflectivity", 1.0f);

  // pointlight
  auto const view_matrix = fstate.view_matrix();
  {
    auto const inv_viewmatrix = glm::inverse(glm::mat3{view_matrix});
    sp.set_uniform_matrix_4fv(logger, "u_invviewmatrix", inv_viewmatrix);
  }

  FOR(i, pointlights.size())
  {
    auto const& transform  = pointlights[i].transform;
    auto const& pointlight = pointlights[i].pointlight;
    set_pointlight(logger, sp, i, pointlight, transform.translation);
  }

  // Material uniforms
  sp.set_uniform_vec3(logger, "u_material.ambient", material.ambient);
  sp.set_uniform_vec3(logger, "u_material.diffuse", material.diffuse);
  sp.set_uniform_vec3(logger, "u_material.specular", material.specular);
  sp.set_uniform_float1(logger, "u_material.shininess", material.shininess);
  // TODO: when re-implementing LOS restrictions
  // sp.set_uniform_vec3(logger, "u_player.position",  player.world_position());
  // sp.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
  // sp.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));
}

} // namespace

namespace opengl
{

void
LightRenderer::set_light_uniforms(RenderState& rstate, EntityRegistry& registry,
                                  ShaderProgram& sp, Material const& material,
                                  glm::vec3 const& position, glm::mat4 const& model_matrix,
                                  bool const set_normalmatrix)
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
