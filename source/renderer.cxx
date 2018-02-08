#include <boomhs/renderer.hpp>
#include <boomhs/state.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/types.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/global.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/shader.hpp>

#include <window/timer.hpp>

#include <stlw/log.hpp>
#include <iostream>

using namespace boomhs;
using namespace opengl;
using namespace window;

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
  glm::mat4 const cam_matrix = camera.camera_matrix();
  auto const mvp_matrix = cam_matrix * model_matrix;

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

void
set_dirlight(stlw::Logger &logger, ShaderProgram &sp, GlobalLight const& global_light)
{
  auto const& directional_light = global_light.directional;
  sp.set_uniform_vec3(logger, "u_dirlight.direction", directional_light.direction);

  auto const& light = directional_light.light;
  sp.set_uniform_color_3fv(logger, "u_dirlight.diffuse", light.diffuse);
  sp.set_uniform_color_3fv(logger, "u_dirlight.specular", light.specular);
}

void
set_pointlight(stlw::Logger &logger, ShaderProgram &sp, std::size_t const index,
    PointLight const& pointlight, glm::vec3 const& pointlight_position)
{
  std::string const varname = "u_pointlights[" + std::to_string(index) + "]";
  auto const make_field = [&varname](char const* fieldname) {
    return varname + "." + fieldname;
  };

  //std::cerr << "pointlight POSITION: '" << glm::to_string(pointlight_position) << "'\n";
  //std::cerr << "pointlight DIFFUSE: '" << pointlight.light.diffuse << "'\n";
  sp.set_uniform_color_3fv(logger, make_field("diffuse"), pointlight.light.diffuse);
  sp.set_uniform_color_3fv(logger, make_field("specular"), pointlight.light.specular);
  sp.set_uniform_vec3(logger, make_field("position"), pointlight_position);

  auto const& attenuation = pointlight.light.attenuation;
  auto const attenuation_field = [&make_field](char const* fieldname) {
    return make_field("attenuation.") + fieldname;
  };
  auto const constant = attenuation_field("constant");
  auto const linear = attenuation_field("linear");
  auto const quadratic = attenuation_field("quadratic");
  //std::cerr << constant << "\n";
  //std::cerr << linear << "\n";
  //std::cerr << quadratic << "\n";
  sp.set_uniform_float1(logger, constant,  attenuation.constant);
  sp.set_uniform_float1(logger, linear,    attenuation.linear);
  sp.set_uniform_float1(logger, quadratic, attenuation.quadratic);
}

void
set_receiveslight_uniforms(boomhs::RenderArgs const &args, glm::mat4 const& model_matrix,
    ShaderProgram &sp, DrawInfo const& dinfo, std::uint32_t const entity,
    entt::DefaultRegistry &registry, bool const receives_ambient_light)
{
  auto const& camera = args.camera;
  auto const& global_light = args.global_light;
  auto const& player = args.player;
  auto &logger = args.logger;

  bool const receives_light = registry.has<Material>(entity);
  assert(receives_light);

  set_modelmatrix(logger, model_matrix, sp);
  sp.set_uniform_matrix_3fv(logger, "u_normalmatrix", glm::inverseTranspose(glm::mat3{model_matrix}));

  //set_dirlight(logger, sp, global_light);

  // ambient
  if (receives_ambient_light) {
    sp.set_uniform_color_3fv(logger, "u_globallight.ambient", global_light.ambient);
  }

  // specular
  sp.set_uniform_float1(logger, "u_reflectivity", 1.0f);

  // pointlight
  sp.set_uniform_matrix_4fv(logger, "u_invviewmatrix", glm::inverse(glm::mat3{camera.view_matrix()}));
  auto const pointlights = find_pointlights(registry);
  //std::cerr << "===============================\n";
  FOR(i, pointlights.size()) {
    auto const& entity = pointlights[i];
    auto &transform = registry.get<Transform>(entity);
    auto &pointlight = registry.get<PointLight>(entity);
    set_pointlight(logger, sp, i, pointlight, transform.translation);
  }

  assert(registry.has<Material>(entity));
  Material const& material = registry.get<Material>(entity);

  sp.set_uniform_vec3(logger, "u_material.ambient",  material.ambient);
  sp.set_uniform_vec3(logger, "u_material.diffuse",  material.diffuse);
  sp.set_uniform_vec3(logger, "u_material.specular", material.specular);
  sp.set_uniform_float1(logger, "u_material.shininess", material.shininess);

  sp.set_uniform_bool(logger, "u_drawnormals", args.draw_normals);
  // TODO: when re-implementing LOS restrictions
  //sp.set_uniform_vec3(logger, "u_player.position",  player.world_position());
  //sp.set_uniform_vec3(logger, "u_player.direction",  player.forward_vector());
  //sp.set_uniform_float1(logger, "u_player.cutoff",  glm::cos(glm::radians(90.0f)));
}

void
set_3dlightsource_uniforms(boomhs::RenderArgs const &args, glm::mat4 const& model_matrix,
    ShaderProgram &sp, DrawInfo const& dinfo, std::uint32_t const entity,
    entt::DefaultRegistry &registry)
{
  auto &logger = args.logger;

  bool const is_lightsource = registry.has<PointLight>(entity);
  assert(is_lightsource);
  auto const pointlights = find_pointlights(registry);

  PointLight *ptr = nullptr;
  FOR(i, pointlights.size()) {
    auto const e = pointlights[i];
    if (entity == e) {
      ptr = &registry.get<PointLight>(e);
      break;
    }
  }
  assert(nullptr != ptr);

  auto const diffuse = ptr->light.diffuse;
  sp.set_uniform_color_3fv(logger, "u_lightcolor", diffuse);
}

void
draw_3dshape(boomhs::RenderArgs const &args, glm::mat4 const& model_matrix, ShaderProgram &sp,
  DrawInfo const& dinfo, std::uint32_t const entity, entt::DefaultRegistry &registry,
  bool const receives_ambient_light)
{
  auto &logger = args.logger;
  auto const& camera = args.camera;

  glm::mat4 const view_matrix = camera.camera_matrix();
  auto const draw_3d_shape_fn = [&](auto const &dinfo) {

    // various matrices
    set_mvpmatrix(logger, model_matrix, sp, camera);

    // We do this assert during load time, but still valid here.
    bool const receives_light = registry.has<Material>(entity);
    bool const is_lightsource = registry.has<PointLight>(entity);

    assert(
        (!receives_light && !is_lightsource) ||
        (receives_light && !is_lightsource) ||
        (!receives_light && is_lightsource));

    if (receives_light) {
      set_receiveslight_uniforms(args, model_matrix, sp, dinfo, entity, registry,
          receives_ambient_light);
    } else if (is_lightsource) {
      set_3dlightsource_uniforms(args, model_matrix, sp, dinfo, entity, registry);
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
  //glDisable(GL_BLEND);

  enable_depth_tests();
}

void
clear_screen(Color const& color)
{
  // https://stackoverflow.com/a/23944124/562174
  glDisable(GL_DEPTH_TEST);
  ON_SCOPE_EXIT([]() { glEnable(GL_DEPTH_TEST); });

  // Render
  glClearColor(color.r(), color.g(), color.b(), color.a());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
draw(RenderArgs const& args, Transform const& transform, ShaderProgram &sp,
    DrawInfo const& dinfo, std::uint32_t const entity, entt::DefaultRegistry &registry)
{
  auto &logger = args.logger;

  // Use the sp's PROGRAM and bind the sp's VAO.
  sp.use(logger);
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
      bool constexpr receives_ambient_light = true;
      draw_3dshape(args, transform.model_matrix(), sp, dinfo, entity, registry,
          receives_ambient_light);
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
draw_tilemap(RenderArgs const& args, DrawTilemapArgs &dt_args, TileMap const& tilemap,
    TilemapState const& tilemap_state, entt::DefaultRegistry &registry, FrameTime const& ft)
{
  auto &logger = args.logger;
  auto const& draw_tile_helper = [&](auto &sp, auto const& dinfo, std::uint32_t const entity,
      glm::mat4 const& model_mat, bool const receives_ambient_light)
  {
    sp.use(logger);
    opengl::global::vao_bind(dinfo.vao());
    ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });
    draw_3dshape(args, model_mat, sp, dinfo, entity, registry, receives_ambient_light);
  };
  auto const draw_tile = [&](auto const& tile_pos) {
    auto const& tile = tilemap.data(tile_pos);
    if (!tilemap_state.reveal && !tile.is_visible) {
      return;
    }
    // This offset causes the tile's to appear in the "middle"
    auto &transform = registry.get<Transform>(tile.eid);
    glm::vec3 constexpr VIEWING_OFFSET{0.5f, 0.0f, 0.5f};

    auto const mmatrix = [&](auto const& translation, auto const& rotation, auto const& scale)
    {
      return stlw::math::calculate_modelmatrix(translation, rotation, scale);
    };

    auto &plus = dt_args.plus;
    auto &hashtag = dt_args.hashtag;
    auto &river = dt_args.river;
    auto &stairs_down = dt_args.stairs_down;
    auto &stairs_up = dt_args.stairs_up;
    switch(tile.type) {
      case TileType::FLOOR:
        {
          auto const translation = tile_pos + VIEWING_OFFSET;
          auto const& rotation   = transform.rotation;
          draw_tile_helper(plus.sp, plus.dinfo, plus.eid, mmatrix(translation, rotation, transform.scale), true);
        }
        break;
      case TileType::WALL:
        {
          auto const translation = tile_pos + VIEWING_OFFSET;
          auto const& rotation   = transform.rotation;
          draw_tile_helper(hashtag.sp, hashtag.dinfo, hashtag.eid, mmatrix(translation, rotation, transform.scale), true);
        }
        break;
      case TileType::RIVER:
        {
          // do nothing, explicitely as we handle drawing rivers separately from the rest of the
          // tilemap.
          //draw_rivers(args, river.sp, river.dinfo, registry, ft, tile.eid, river.eid);
          break;
        }
        break;
      case TileType::STAIR_DOWN:
        {
          auto &sp = stairs_down.sp;
          sp.set_uniform_color(logger, "u_color", LOC::GREEN);

          auto const translation = tile_pos + VIEWING_OFFSET;
          auto const& rotation   = transform.rotation;

          bool const receives_ambient_light = false;
          glm::vec3 constexpr scale{1.0f, 1.0f, 1.0f};
          auto const model_matrix = mmatrix(translation, rotation, scale);
          draw_tile_helper(sp, stairs_down.dinfo, stairs_down.eid, model_matrix, receives_ambient_light);
        }
        break;
      case TileType::STAIR_UP:
        {
          auto &sp = stairs_up.sp;
          sp.set_uniform_color(logger, "u_color", LOC::RED);

          auto const translation = tile_pos + VIEWING_OFFSET;
          auto const& rotation   = transform.rotation;

          bool const receives_ambient_light = false;
          glm::vec3 constexpr scale{1.0f, 1.0f, 1.0f};
          auto const model_matrix = mmatrix(translation, rotation, scale);
          draw_tile_helper(sp, stairs_up.dinfo, stairs_up.eid, model_matrix, receives_ambient_light);
        }
        break;
      default:
        std::exit(1);
    }
  };
  tilemap.visit_each(draw_tile);
}

void
draw_rivers(RenderArgs const& rargs, opengl::ShaderProgram & sp, opengl::DrawInfo const& dinfo,
    entt::DefaultRegistry &registry, window::FrameTime const& ft, uint32_t const river_eid,
    RiverInfo &rinfo)
{
  auto const abscos = [&rinfo](float const v) {
    return std::abs(std::cos(v));
  };

  auto const& left = rinfo.left;
  auto const& right = rinfo.right;

  sp.use(rargs.logger);

  auto const draw_wiggle = [&](auto &wiggle) {
    auto &x = wiggle.position.x;
    x += abscos(ft.delta) / wiggle.speed;
    if (x > right.x) {
      x = left.x;
    }

    auto const u_zoffset = abscos(ft.delta * wiggle.z_jiggle);
    sp.set_uniform_float1(rargs.logger, "u_zoffset", u_zoffset);
    sp.set_uniform_color(rargs.logger, "u_color", LOC::BLUE);
    opengl::global::vao_bind(dinfo.vao());
    ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

    auto const tr = wiggle.position;
    auto const rot = glm::quat{};
    auto const scale = glm::vec3{1.0};

    bool const receives_ambient = false;
    auto const modelmatrix = stlw::math::calculate_modelmatrix(tr, rot, scale);
    draw_3dshape(rargs, modelmatrix, sp, dinfo, river_eid, registry, receives_ambient);
  };
  for (auto &w : rinfo.wiggles) {
    draw_wiggle(w);
  }
}

void
draw_tilegrid(RenderArgs const& args, Transform const& transform, ShaderProgram &sp,
    DrawInfo const& dinfo)
{
  auto &logger = args.logger;
  sp.use(logger);
  opengl::global::vao_bind(dinfo.vao());
  ON_SCOPE_EXIT([]() { opengl::global::vao_unbind(); });

  set_mvpmatrix(logger, transform.model_matrix(), sp, args.camera);
  draw_drawinfo(logger, sp, dinfo);
}

} // ns boomhs::render
