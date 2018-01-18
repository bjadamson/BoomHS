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
set_modelmatrix(stlw::Logger &logger, glm::mat4 const& model_matrix, ShaderProgram &shader_program)
{
  shader_program.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
}

void
set_mvpmatrix(stlw::Logger &logger, glm::mat4 const& model_matrix, ShaderProgram &shader_program,
    Camera const& camera)
{
  glm::mat4 const view_matrix = camera.camera_matrix();
  auto const mvp_matrix = view_matrix * model_matrix;

  shader_program.set_uniform_matrix_4fv(logger, "u_mvpmatrix", mvp_matrix);
}

void
draw_drawinfo(stlw::Logger &logger, ShaderProgram &shader_program, DrawInfo const& dinfo)
{
  auto const draw_mode = dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET = nullptr;

  if (shader_program.instance_count) {
    auto const ic = *shader_program.instance_count;
    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, ic);
  } else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
  }
}

void
draw_3dshape(boomhs::RenderArgs const &args, glm::mat4 const& model_matrix, ShaderProgram &shader_program,
    DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  auto const& camera = args.camera;
  auto const& player = args.player.world_object;
  auto const& player_material = args.player.material;
  auto const& light = args.light;

  glm::mat4 const view_matrix = camera.camera_matrix();
  auto const draw_3d_shape_fn = [&](auto const &dinfo) {

    // various matrices
    set_mvpmatrix(logger, model_matrix, shader_program, camera);

    if (shader_program.receives_light) {
      set_modelmatrix(logger, model_matrix, shader_program);
      //shader_program.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
      shader_program.set_uniform_vec3(logger, "u_viewpos", camera.world_position());
      {
        auto const light_pos = args.light.single_light_position;
        shader_program.set_uniform_vec3(logger, "u_light.position", light_pos);
      }
      shader_program.set_uniform_color_3fv(logger, "u_light.ambient", light.ambient);
      shader_program.set_uniform_color_3fv(logger, "u_light.diffuse", light.diffuse);
      shader_program.set_uniform_color_3fv(logger, "u_light.specular", light.specular);

      shader_program.set_uniform_float1(logger, "u_light.constant",  light.attenuation.constant);
      shader_program.set_uniform_float1(logger, "u_light.linear",    light.attenuation.linear);
      shader_program.set_uniform_float1(logger, "u_light.quadratic", light.attenuation.quadratic);

      shader_program.set_uniform_vec3(logger, "u_material.ambient",  player_material.ambient);
      shader_program.set_uniform_vec3(logger, "u_material.diffuse",  player_material.diffuse);
      shader_program.set_uniform_vec3(logger, "u_material.specular", player_material.specular);
      shader_program.set_uniform_float1(logger, "u_material.shininess", player_material.shininess);

      shader_program.set_uniform_vec3(logger, "u_player.position",  player.world_position());
      shader_program.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
      shader_program.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));
    }

    if (shader_program.is_skybox) {
      render::disable_depth_tests();
      draw_drawinfo(logger, shader_program, dinfo);
      render::enable_depth_tests();
    } else {
      draw_drawinfo(logger, shader_program, dinfo);
    }
  };

  if (dinfo.texture_info()) {
    auto const& ti = *dinfo.texture_info();
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
draw(RenderArgs const& args, Transform const& transform, ShaderProgram &shader_program,
    DrawInfo const& dinfo)
{
  auto &logger = args.logger;

  // Use the shader_program's PROGRAM and bind the shader_program's VAO.
  shader_program.use_program(logger);
  opengl::global::vao_bind(dinfo.vao());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinfo.ebo());
  glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
  /*
  std::cerr << "---------------------------------------------------------------------------\n";
  std::cerr << "drawing object!\n";
  std::cerr << "shader_program:\n" << shader_program << "\n";

  std::cerr << "draw_info:\n";
  dinfo.print_self(std::cerr, shader_program.va());
  std::cerr << "\n";
  std::cerr << "---------------------------------------------------------------------------\n";
  */

  auto const draw_fn = [&]()
  {
    if (shader_program.is_2d) {
      disable_depth_tests();
      set_modelmatrix(logger, transform.model_matrix(), shader_program);
      enable_depth_tests();
    } else {
      draw_3dshape(args, transform.model_matrix(), shader_program, dinfo);
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
    TileMap const& tilemap, bool const reveal_map)
{
  auto const& draw_tile_helper = [&](auto &shader_program, auto const& dinfo, glm::vec3 const& tile_pos) {
    auto &logger = args.logger;
    shader_program.use_program(logger);
    opengl::global::vao_bind(dinfo.vao());

    glm::mat4 const model_matrix = transform.model_matrix();
    glm::mat4 const translated = glm::translate(model_matrix, tile_pos);
    draw_3dshape(args, translated, shader_program, dinfo);
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
draw_tilegrid(RenderArgs const& args, Transform const& transform, ShaderProgram &shader_program,
    DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  shader_program.use_program(logger);
  opengl::global::vao_bind(dinfo.vao());

  set_mvpmatrix(logger, transform.model_matrix(), shader_program, args.camera);
  draw_drawinfo(logger, shader_program, dinfo);
}

} // ns boomhs::render
