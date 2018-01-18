#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/types.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/global.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>

#include <stlw/log.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

using namespace opengl;
using namespace boomhs;

namespace
{

void
set_modelmatrix(stlw::Logger &logger, glm::mat4 const& model_matrix, ShaderProgram &sp)
{
  sp.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
}

void
set_mvpmatrix(stlw::Logger &logger, glm::mat4 const& model_matrix, ShaderProgram &sp,
    Camera const& camera)
{
  glm::mat4 const view_matrix = camera.camera_matrix();
  auto const mvp_matrix = view_matrix * model_matrix;

  sp.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
}

void
draw_drawinfo(stlw::Logger &logger, ShaderProgram &sp, DrawInfo const& dinfo)
{
  auto const draw_mode = dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET = nullptr;

  if (sp.instance_count) {
    auto const ic = *sp.instance_count;
    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, ic);
  } else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
  }
}

/*
void
set_dirlight(stlw::Logger &logger, ShaderProgram &sp, GlobalLight const& global_light)
{
  auto const light_pos = global_light.directional_light_position;
  auto const& directional_light = global_light.directional_light;
  sp.set_uniform_vec3(logger, "u_dirlight.position", light_pos);

  sp.set_uniform_color_3fv(logger, "u_dirlight.diffuse", directional_light.diffuse);
  sp.set_uniform_color_3fv(logger, "u_dirlight.specular", directional_light.specular);

  auto const& attenuation = directional_light.attenuation;
  sp.set_uniform_float1(logger, "u_dirlight.attenuation.constant",  attenuation.constant);
  sp.set_uniform_float1(logger, "u_dirlight.attenuation.linear",    attenuation.linear);
  sp.set_uniform_float1(logger, "u_dirlight.attenuation.quadratic", attenuation.quadratic);
}
*/

std::vector<uint32_t>
find_pointlights(entt::DefaultRegistry &registry)
{
  // for now assume only 1 entity has the Player tag
  assert(PointLights::MAX_NUMBER_POINTLIGHTS >= registry.view<PointLight>().size());

  std::vector<std::uint32_t> point_lights;
  auto view = registry.view<PointLight, Transform>();
  for (auto const entity : view) {
    point_lights.emplace_back(entity);
  }
  std::cerr << "found '" << point_lights.size() << "' point lights\n";
  return point_lights;
}

void
set_pointlight(stlw::Logger &logger, ShaderProgram &sp, std::size_t const index,
    PointLight const& pointlight, glm::vec3 const& pointlight_position)
{
  std::string const varname = "u_pointlights[" + std::to_string(index) + "]";
  auto const make_field = [&varname](char const* fieldname) {
    return varname + "." + fieldname;
  };

  std::cerr << "pointlight POSITION: '" << glm::to_string(pointlight_position) << "'\n";
  std::cerr << "pointlight DIFFUSE: '" << pointlight.light.diffuse << "'\n";

  auto const diffuse = make_field("diffuse");
  auto const specular = make_field("specular");
  sp.set_uniform_color_3fv(logger, diffuse, pointlight.light.diffuse);
  sp.set_uniform_color_3fv(logger, specular, pointlight.light.specular);

  auto const& attenuation = pointlight.light.attenuation;
  auto const attenuation_field = [&make_field](char const* fieldname) {
    return make_field("attenuation.") + fieldname;
  };
  auto const constant = attenuation_field("constant");
  auto const linear = attenuation_field("constant");
  auto const quadratic = attenuation_field("constant");
  std::cerr << constant << "\n";
  std::cerr << linear << "\n";
  std::cerr << quadratic << "\n";
  sp.set_uniform_float1(logger, constant,  attenuation.constant);
  sp.set_uniform_float1(logger, linear,    attenuation.linear);
  sp.set_uniform_float1(logger, quadratic, attenuation.quadratic);
}

void
draw_3dwithlighting(boomhs::RenderArgs const &args, glm::mat4 const& model_matrix,
    ShaderProgram &sp, DrawInfo const& dinfo, entt::DefaultRegistry &registry)
{
  auto const& camera = args.camera;
  auto const& global_light = args.global_light;
  auto const& player = args.player.world_object;
  auto const& player_material = args.player.material;
  auto &logger = args.logger;

  assert(sp.receives_light);

  set_modelmatrix(logger, model_matrix, sp);
  sp.set_uniform_vec3(logger, "u_viewpos", camera.world_position());
  sp.set_uniform_color_3fv(logger, "u_globallight.ambient", global_light.ambient);

  //set_dirlight(logger, sp, global_light);

  auto const pointlights = find_pointlights(registry);
  FOR(i, pointlights.size()) {
    auto const& entity = pointlights[i];
    auto &transform = registry.get<Transform>(entity);
    auto &pointlight = registry.get<PointLight>(entity);
    set_pointlight(logger, sp, i, pointlight, transform.translation);
  }
  std::cerr << "------------------------------------------------------------\n\n\n";

  sp.set_uniform_vec3(logger, "u_material.ambient",  player_material.ambient);
  sp.set_uniform_vec3(logger, "u_material.diffuse",  player_material.diffuse);
  sp.set_uniform_vec3(logger, "u_material.specular", player_material.specular);
  sp.set_uniform_float1(logger, "u_material.shininess", player_material.shininess);

  sp.set_uniform_vec3(logger, "u_player.position",  player.world_position());
  sp.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
  sp.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));
}

void
draw_3dshape(boomhs::RenderArgs const &args, glm::mat4 const& model_matrix,
    ShaderProgram &sp, DrawInfo const& dinfo, entt::DefaultRegistry &registry)
{
  auto &logger = args.logger;
  auto const& camera = args.camera;

  glm::mat4 const view_matrix = camera.camera_matrix();
  auto const draw_3d_shape_fn = [&](auto const &dinfo) {

    // various matrices
    set_mvpmatrix(logger, model_matrix, sp, camera);

    if (sp.receives_light) {
      draw_3dwithlighting(args, model_matrix, sp, dinfo, registry);
    }

    if (sp.is_skybox) {
      render::disable_depth_tests();
      draw_drawinfo(logger, sp, dinfo);
      render::enable_depth_tests();
    } else {
      draw_drawinfo(logger, sp, dinfo);
    }
  };

  if (dinfo.texture_info()) {
    auto const ti = *dinfo.texture_info();
    opengl::global::texture_bind(ti);
    ON_SCOPE_EXIT([&ti]() { opengl::global::texture_unbind(ti); });
    draw_3d_shape_fn(dinfo);
  } else {
    draw_3d_shape_fn(dinfo);
  }
}

} // ns anonymous

namespace boomhs::render
{

void
enable_depth_tests()
{
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}

void
disable_depth_tests()
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
}

void
init(window::Dimensions const& dimensions)
{
  // Initialize opengl
  glViewport(0, 0, dimensions.w, dimensions.h);

  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  enable_depth_tests();
}

void
clear_screen(Color const& color)
{
  // https://stackoverflow.com/a/23944124/562174
  glDisable(GL_DEPTH_TEST);
  ON_SCOPE_EXIT([]() { glEnable(GL_DEPTH_TEST); });

  // Render
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
draw(RenderArgs const& args, Transform const& transform, ShaderProgram &sp,
    DrawInfo const& dinfo, entt::DefaultRegistry &registry)
{
  auto &logger = args.logger;

  // Use the sp's PROGRAM and bind the sp's VAO.
  sp.use_program(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

  /*
  std::cerr << "---------------------------------------------------------------------------\n";
  std::cerr << "drawing object!\n";
  std::cerr << "sp:\n" << sp << "\n";

  std::cerr << "draw_info:\n";
  dinfo.print_self(std::cerr, sp.va());
  std::cerr << "\n";
  std::cerr << "---------------------------------------------------------------------------\n";
  */

  auto const draw_fn = [&]()
  {
    if (sp.is_2d) {
      disable_depth_tests();
      set_modelmatrix(logger, transform.model_matrix(), sp);
      enable_depth_tests();
    } else {
      draw_3dshape(args, transform.model_matrix(), sp, dinfo, registry);
    }
  };

  if (dinfo.texture_info()) {
    auto const& ti = *dinfo.texture_info();
    opengl::global::texture_bind(ti);
    ON_SCOPE_EXIT([&ti]() { opengl::global::texture_unbind(ti); });
    draw_fn();
  } else {
    draw_fn();
  }
}

void
draw_tilemap(RenderArgs const& args, Transform const& transform, DrawTilemapArgs &&dt_args,
    TileMap const& tilemap, bool const reveal_map, entt::DefaultRegistry &registry)
{
  auto const& draw_tile_helper = [&](auto &sp, auto const& dinfo, glm::vec3 const& tile_pos) {
    auto &logger = args.logger;
    sp.use_program(logger);
    opengl::global::vao_bind(dinfo.vao());
    ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

    glm::mat4 const model_matrix = transform.model_matrix();
    glm::mat4 const translated = glm::translate(model_matrix, tile_pos);
    draw_3dshape(args, translated, sp, dinfo, registry);
  };

  auto const draw_all_tiles = [&](auto const& pos) {
    auto const& tile = tilemap.data(pos.x, pos.y, pos.z);
    if (!reveal_map && !tile.is_visible) {
      return;
    }
    if(tile.is_wall) {
      draw_tile_helper(dt_args.hashtag_shader_program, dt_args.hashtag_dinfo, pos);
    } else {
      draw_tile_helper(dt_args.plus_shader_program, dt_args.plus_dinfo, pos);
    }
  };
  tilemap.visit_each(draw_all_tiles);
}

void
draw_tilegrid(RenderArgs const& args, Transform const& transform, ShaderProgram &sp,
    DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  sp.use_program(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

  set_mvpmatrix(logger, transform.model_matrix(), sp, args.camera);
  draw_drawinfo(logger, sp, dinfo);
}

} // ns boomhs::render
