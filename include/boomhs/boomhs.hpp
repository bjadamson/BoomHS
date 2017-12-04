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
#include <opengl/skybox.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/burrito.hpp>
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

struct RenderArgs {
  Logger &logger;
  opengl::camera const& camera;
  glm::mat4 const& projection;
};

struct GameState {
  bool quit = false;
  Logger &logger;
  window::Dimensions const dimensions;
  stlw::float_generator rnum_generator;
  glm::mat4 projection;
  std::vector<::opengl::Model*> MODELS;
  opengl::Model skybox_model;
  opengl::Model house_model;
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

  RenderArgs render_args() const
  {
    return RenderArgs{this->logger, this->camera, this->projection};
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
  auto constexpr WIDTH = NUM_TILES / 100;
  assert((NUM_TILES % WIDTH) == 0);

  auto tile_vec = std::vector<Tile>{};
  tile_vec.reserve(NUM_TILES);

  std::size_t count = 0;
  while(count < NUM_TILES) {
    auto constexpr y = 0.0;
    auto Z = 0.0;
    FOR(x, 50) {
      Z = 0.0;
      FOR(_, 20) {
        Tile tile{glm::vec3{x, y, Z}};
        tile_vec.emplace_back(MOVE(tile));
        Z += 1.0f;
        ++count;
      }
    }
  }
  assert(tile_vec.size() == NUM_TILES);
  auto tmap = TileMap{MOVE(tile_vec), WIDTH};
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
    auto const [x, y] = state.rnum_generator.generate_2dposition();
    auto constexpr Z = 0.0f;
    auto const translation = glm::vec3{x, y, Z};
    state.MODELS.emplace_back(make_entity(count++, translation));
  }

  // LOAD different assets.
  //"assets/chalet.mtl"
  namespace OF = opengl::factories;
  auto &logger = state.logger;

  auto house_obj = opengl::load_mesh("assets/house_uv.obj", opengl::LoadNormals{true}, opengl::LoadUvs{true});
  auto hashtag_obj = opengl::load_mesh("assets/hashtag.obj", "assets/hashtag.mtl", opengl::LoadNormals{false}, opengl::LoadUvs{false});
  Objs objs{MOVE(house_obj), MOVE(hashtag_obj)};

  auto house = OF::make_mesh(logger, gfx.d3.house,
      opengl::mesh_properties{GL_TRIANGLES, objs.house});
  auto hashtag = OF::make_mesh(logger, gfx.d3.hashtag,
      opengl::mesh_properties{GL_TRIANGLES, objs.hashtag});

  auto cube_skybox = OF::copy_cube_gpu(logger, gfx.d3.skybox, {GL_TRIANGLE_STRIP, {10.0f, 10.0f, 10.0f}});
  auto cube_textured = OF::copy_cube_gpu(logger, gfx.d3.texture_cube, {GL_TRIANGLE_STRIP,
      {0.15f, 0.15f, 0.15f}});
  auto cube_colored = OF::copy_cube_gpu(logger, gfx.d3.color, {GL_TRIANGLE_STRIP, {0.25f, 0.25f, 0.25f}},
      opengl::LIST_OF_COLORS::BLUE);

  auto cube_wf = OF::copy_cube_gpu(logger, gfx.d3.wireframe, {GL_LINE_LOOP, {0.25f, 0.25f, 0.25f}});

  auto terrain_handle = OF::copy_cube_gpu(logger, gfx.d3.color, {GL_TRIANGLE_STRIP, {2.0f, 0.4f, 2.0f}},
      opengl::LIST_OF_COLORS::SADDLE_BROWN);

  auto tilemap_handle = OF::copy_tilemap_gpu(logger, gfx.d3.hashtag,
      {GL_TRIANGLES, objs.hashtag.vertices, objs.hashtag.indices},
      state.tilemap);

  GpuHandles handles{MOVE(house), MOVE(hashtag), MOVE(cube_skybox), MOVE(cube_textured),
    MOVE(cube_colored), MOVE(cube_wf), MOVE(terrain_handle), MOVE(tilemap_handle)};
  return Assets{MOVE(objs), MOVE(handles)};
}

void game_loop(GameState &state, opengl::OpenglPipelines &gfx, Assets const& assets)
{
  {
    auto const color = opengl::LIST_OF_COLORS::WHITE;
    glm::vec4 const vec4 = {color[0], color[1], color[2], 1.0};
    opengl::clear_screen(vec4);
  }

  auto render_args = state.render_args();
  //auto &logger = state.logger;

  // first, draw terrain
  //opengl::draw(render_args, state.terrain_model, gfx.d3.color, assets.handles.terrain);

  // now draw entities
  opengl::draw(render_args, state.skybox_model, gfx.d3.skybox, assets.handles.cube_skybox);

  opengl::draw(render_args, *state.MODELS[0], gfx.d3.color, assets.handles.cube_colored);
  opengl::draw(render_args, *state.MODELS[1], gfx.d3.texture_cube, assets.handles.cube_textured);
  opengl::draw(render_args, *state.MODELS[2], gfx.d3.wireframe, assets.handles.cube_wireframe);

  // house
  opengl::draw(render_args, state.house_model, gfx.d3.house, assets.handles.house);

  // tilemap
  opengl::draw_tilemap(render_args, *state.MODELS[3], gfx.d3.hashtag,
      assets.handles.tilemap, state.tilemap);
}

} // ns boomhs
