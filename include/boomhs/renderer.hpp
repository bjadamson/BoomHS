#pragma once
#include <opengl/global.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boomhs/state.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

namespace boomhs::render {

inline void
enable_depth_tests()
{
  //glCullFace(GL_BACK);
  //glFrontFace(GL_CW);
  //glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
}

inline void
disable_depth_tests()
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
}

inline void
init(window::Dimensions const& dimensions)
{
  // Initialize opengl
  glViewport(0, 0, dimensions.w, dimensions.h);

  glDisable(GL_CULL_FACE);
  enable_depth_tests();
}

inline void
clear_screen(opengl::Color const& color)
{
  glClearColor(color.r, color.g, color.b, color.a);

  // Render
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

namespace detail {

template <typename SP, typename FN>
void
bind_stuff_and_draw(stlw::Logger &logger, SP &shader_program, opengl::DrawInfo const &dinfo, FN const& fn)
{
  using namespace opengl;

  if constexpr (SP::HAS_TEXTURE) {
    opengl::global::texture_bind(shader_program.texture());
    ON_SCOPE_EXIT([&shader_program]() { opengl::global::texture_unbind(shader_program.texture()); });
    fn(dinfo);
  } else {
    fn(dinfo);
  }
}

template <typename SP>
void render_element_buffer(stlw::Logger &logger, SP &shader_program, opengl::DrawInfo const& dinfo)
{
  auto const draw_mode = dinfo.draw_mode();
  auto const num_indices = dinfo.num_indices();
  auto constexpr OFFSET = nullptr;

  if constexpr (SP::IS_INSTANCED) {
    auto const instance_count = shader_program.instance_count();
    glDrawElementsInstanced(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr, instance_count);
  } else {
    glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, OFFSET);
  }
}

template <typename SP>
void
draw_3dshape(RenderArgs const &args, glm::mat4 const& model_matrix, SP &shader_program, opengl::DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  auto const& camera = args.camera;
  auto const& player = args.player;
  auto const& light = args.light;
  auto const& at_materials = args.at_materials;

  glm::mat4 const view_matrix = camera.camera_matrix();

  auto const draw_3d_shape_fn = [&](auto const &dinfo) {

    // various matrices
    shader_program.set_uniform_matrix_4fv(logger, "u_mvpmatrix", view_matrix * model_matrix);

    if constexpr (SP::RECEIVES_LIGHT) {
      shader_program.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
      shader_program.set_uniform_vec3(logger, "u_viewpos", camera.world_position());
      {
        auto const light_pos = args.entities[LIGHT_INDEX]->translation;
        shader_program.set_uniform_vec3(logger, "u_light.position", light_pos);
      }
      shader_program.set_uniform_color_3fv(logger, "u_light.ambient", light.ambient);
      shader_program.set_uniform_color_3fv(logger, "u_light.diffuse", light.diffuse);
      shader_program.set_uniform_color_3fv(logger, "u_light.specular", light.specular);

      shader_program.set_uniform_float1(logger, "u_light.constant",  light.attenuation.constant);
      shader_program.set_uniform_float1(logger, "u_light.linear",    light.attenuation.linear);
      shader_program.set_uniform_float1(logger, "u_light.quadratic", light.attenuation.quadratic);

      shader_program.set_uniform_color_3fv(logger, "u_material.ambient",  at_materials.ambient);
      shader_program.set_uniform_color_3fv(logger, "u_material.diffuse",  at_materials.diffuse);
      shader_program.set_uniform_color_3fv(logger, "u_material.specular", at_materials.specular);
      shader_program.set_uniform_float1(logger, "u_material.shininess", at_materials.shininess);

      shader_program.set_uniform_vec3(logger, "u_player.position",  player.world_position());
      shader_program.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
      shader_program.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(45.0f)));
    }

    if constexpr (SP::IS_SKYBOX) {
      disable_depth_tests();
      render_element_buffer(logger, shader_program, dinfo);
      enable_depth_tests();
    } else {
      render_element_buffer(logger, shader_program, dinfo);
    }
  };

  bind_stuff_and_draw(logger, shader_program, dinfo, draw_3d_shape_fn);
}

template <typename SP>
void
draw_2dshape(RenderArgs const &args, boomhs::Transform const& transform, SP &shader_program, opengl::DrawInfo const &dinfo)
{
  using namespace opengl;

  auto &logger = args.logger;
  auto const draw_2d_shape_fn = [&](auto const& dinfo) {
    auto const model_matrix = transform.model_matrix();
    shader_program.set_uniform_matrix_4fv(logger, "u_modelmatrix", model_matrix);
    render_element_buffer(logger, shader_program, dinfo);
  };

  bind_stuff_and_draw(logger, shader_program, dinfo, draw_2d_shape_fn);
}

} // ns detail

template <typename SP>
void
draw(RenderArgs const& args, Transform const& transform, SP &shader_program, opengl::DrawInfo const& dinfo)
{
  auto &logger = args.logger;

  // Use the shader_program's PROGRAM and bind the shader_program's VAO.
  shader_program.use_program(logger);
  opengl::global::vao_bind(dinfo.vao());

  if constexpr (SP::IS_2D) {
    disable_depth_tests();
    detail::draw_2dshape(args, transform, shader_program, dinfo);
    enable_depth_tests();
  } else {
    auto const model_matrix = transform.model_matrix();
    detail::draw_3dshape(args, model_matrix, shader_program, dinfo);
  }
}

struct DrawTilemapArgs
{
  opengl::DrawInfo const& hashtag_dinfo;
  opengl::ShaderProgramHashtag3D &hashtag_shader_program;

  opengl::DrawInfo const& plus_dinfo;
  opengl::ShaderProgramPlus3D &plus_shader_program;
};

template<typename TILEMAP>
void
draw_tilemap(RenderArgs const& args, Transform const& transform, DrawTilemapArgs &&dt_args,
    TILEMAP const& tilemap, bool const reveal_map)
{
  auto const& draw_tile_helper = [&](auto &shader_program, auto const& dinfo, glm::vec3 const& tile_pos) {
    auto &logger = args.logger;
    shader_program.use_program(logger);
    opengl::global::vao_bind(dinfo.vao());

    glm::mat4 const model_matrix = transform.model_matrix();
    glm::mat4 const translated = glm::translate(model_matrix, tile_pos);
    detail::draw_3dshape(args, translated, shader_program, dinfo);
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

template<typename SP>
struct Drawable
{
  SP &shader_program;
  opengl::DrawInfo const& dinfo;
};

inline void
draw_tilegrid(RenderArgs const& args, Transform const& transform,
    Drawable<opengl::ShaderProgramPositionColor3D> &&drawable)
{
  glm::mat4 const view_matrix = args.camera.camera_matrix();
  auto const model_matrix = transform.model_matrix();
  auto const mvp_matrix = view_matrix * model_matrix;

  auto &logger = args.logger;
  auto &shader_program = drawable.shader_program;
  auto &dinfo = drawable.dinfo;
  shader_program.use_program(logger);
  opengl::global::vao_bind(dinfo.vao());

  detail::render_element_buffer(logger, shader_program, dinfo);
}

} // ns boomhs::render
