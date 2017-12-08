#pragma once
#include <string>
#include <tuple>
#include <vector>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <window/mouse.hpp>
#include <window/sdl_window.hpp>
#include <opengl/camera.hpp>
#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/io_system.hpp>
#include <boomhs/randompos_system.hpp>
#include <boomhs/tilemap.hpp>

using stlw::Logger;

namespace boomhs
{

struct GameState {
  bool quit = false;
  Logger &logger;
  window::Dimensions const dimensions;
  stlw::float_generator rnum_generator;
  glm::mat4 projection;

  std::vector<::opengl::Model*> MODELS;
  opengl::Model color_cube_model;


  opengl::Model skybox_model;
  opengl::Model house_model;
  opengl::Model tilemap_model;
  opengl::Model terrain_model;
  opengl::camera camera;

  TileMap tilemap;
  window::mouse_data mouse_data;

public:
  MOVE_CONSTRUCTIBLE_ONLY(GameState);
  GameState(Logger &l, window::Dimensions const &d, stlw::float_generator &&fg,
      glm::mat4 &&pm, TileMap &&tmap)
    : logger(l)
    , dimensions(d)
    , rnum_generator(MOVE(fg))
    , projection(MOVE(pm))
    , camera(opengl::camera_factory::make_default(this->skybox_model))
    , tilemap(MOVE(tmap))
    , mouse_data(window::make_default_mouse_data())
  {
    camera.move_down(1);
  }

  opengl::RenderArgs render_args() const
  {
    return opengl::RenderArgs{this->logger, this->camera, this->projection};
  }
};

auto
make_state(Logger &logger, window::Dimensions const& hw)
{
  auto const fheight = static_cast<GLfloat>(hw.h);
  auto const fwidth = static_cast<GLfloat>(hw.w);
  auto const aspect = fwidth / fheight;

  auto projection = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 200.0f);
  stlw::float_generator rng;

  auto constexpr NUM_TILES = 1000;
  auto const [W, H, L] = std::array<std::size_t, 3>{10, 10, 10};
  assert((W * H * L) == NUM_TILES);

  auto tile_vec = std::vector<Tile>{};
  tile_vec.reserve(NUM_TILES);

  FOR(x, W) {
    FOR(y, H) {
      FOR(z, L) {
        tile_vec.emplace_back(Tile{});
      }
    }
  }

  assert(tile_vec.capacity() == tile_vec.size());
  assert(tile_vec.size() == NUM_TILES);
  auto tmap = TileMap{MOVE(tile_vec), W, H, L};
  return GameState(logger, hw, MOVE(rng), MOVE(projection), MOVE(tmap));
}

auto ecst_systems()
{
  return std::make_tuple(st::io_system, st::randompos_system);
}

template<typename PROXY>
auto
init(PROXY &proxy, GameState &state, opengl::OpenglPipelines &gfx)
{
  auto const make_entity = [&proxy](auto const i, auto const& t) {
    auto eid = proxy.create_entity();
    auto *p = &proxy.add_component(ct::model, eid);
    p->translation = t;
    return p;
  };
  auto count = 0u;

  // The 2D objects
  while(count < 100) {
    auto const [x, y, z] = state.rnum_generator.generate_3dposition_above_ground();
    auto const translation = glm::vec3{x, y, z};
    state.MODELS.emplace_back(make_entity(count++, translation));
  }

  state.color_cube_model.translation = glm::vec3{2.0f, 2.0f, 2.0f};
  state.house_model.translation = glm::vec3{-2.0f, 0.0f, -2.0f};
  state.tilemap_model.translation = glm::vec3{0.0f, 0.0f, 0.0f};
  state.terrain_model.translation = glm::vec3{0.0f, 5.0f, 0.0f};

  // LOAD different assets.
  //"assets/chalet.mtl"
  namespace OF = opengl::factories;
  auto &logger = state.logger;

  auto house_obj = opengl::load_mesh("assets/house_uv.obj", opengl::LoadNormals{true}, opengl::LoadUvs{true});
  auto hashtag_obj = opengl::load_mesh("assets/hashtag.obj", "assets/hashtag.mtl", opengl::LoadNormals{false}, opengl::LoadUvs{false});
  auto at_obj = opengl::load_mesh("assets/at.obj", "assets/at.mtl", opengl::LoadNormals{false}, opengl::LoadUvs{false});
  Objs objs{MOVE(house_obj), MOVE(hashtag_obj), MOVE(at_obj)};

  auto house_handle = OF::make_mesh(logger, gfx.d3.house,
      opengl::MeshProperties{GL_TRIANGLES, objs.house});
  auto hashtag_handle = OF::make_mesh(logger, gfx.d3.hashtag,
      opengl::MeshProperties{GL_TRIANGLES, objs.hashtag});
  auto at_handle = OF::make_mesh(logger, gfx.d3.at,
      opengl::MeshProperties{GL_TRIANGLES, objs.at});

  auto cube_skybox = OF::copy_cube_gpu(logger, gfx.d3.skybox, {GL_TRIANGLE_STRIP, {10.0f, 10.0f, 10.0f}});
  auto cube_textured = OF::copy_cube_gpu(logger, gfx.d3.texture_cube, {GL_TRIANGLE_STRIP,
      {0.15f, 0.15f, 0.15f}});

  auto cube_colored = OF::copy_cube_gpu(logger, gfx.d3.color, {GL_TRIANGLE_STRIP, {0.25f, 0.25f, 0.25f}},
      opengl::LIST_OF_COLORS::BLUE);

  auto cube_wf = OF::copy_cube_gpu(logger, gfx.d3.wireframe, {GL_LINE_LOOP, {0.25f, 0.25f, 0.25f}});

  auto terrain_handle = OF::copy_cube_gpu(logger, gfx.d3.terrain, {GL_TRIANGLE_STRIP, {2.0f, 0.4f, 2.0f}},
      opengl::LIST_OF_COLORS::SADDLE_BROWN);

  auto tilemap_handle = OF::copy_tilemap_gpu(logger, gfx.d3.hashtag,
      {GL_TRIANGLES, objs.hashtag.vertices, objs.hashtag.indices},
      state.tilemap);

  GpuHandles handles{
    MOVE(house_handle),
    MOVE(hashtag_handle),
    MOVE(at_handle),
    MOVE(cube_skybox),
    MOVE(cube_textured),
    MOVE(cube_colored),
    MOVE(cube_wf),
    MOVE(terrain_handle),
    MOVE(tilemap_handle)};
  return Assets{MOVE(objs), MOVE(handles)};
}

void game_loop(GameState &state, opengl::OpenglPipelines &gfx, Assets const& assets)
{
  opengl::clear_screen(opengl::LIST_OF_COLORS::BLACK);
  auto render_args = state.render_args();

  // skybox
  opengl::draw(render_args, state.skybox_model, gfx.d3.skybox, assets.handles.cube_skybox);

  // random
  opengl::draw(render_args, state.color_cube_model, gfx.d3.color, assets.handles.cube_colored);
  opengl::draw(render_args, *state.MODELS[1], gfx.d3.texture_cube, assets.handles.cube_textured);
  opengl::draw(render_args, *state.MODELS[2], gfx.d3.wireframe, assets.handles.cube_wireframe);

  // house
  opengl::draw(render_args, state.house_model, gfx.d3.house, assets.handles.house);

  // tilemap
  opengl::draw_tilemap(render_args, state.tilemap_model, gfx.d3.hashtag,
      assets.handles.tilemap, state.tilemap);

  // terrain
  opengl::draw(render_args, state.terrain_model, gfx.d3.terrain, assets.handles.terrain);
}

} // ns boomhs
