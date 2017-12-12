#pragma once

#include <imgui/imgui.hpp>

#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/io_system.hpp>
#include <boomhs/randompos_system.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui.hpp>

#include <stlw/log.hpp>

#include <vector>

using stlw::Logger;

namespace boomhs
{

auto ecst_systems()
{
  return std::make_tuple(st::io_system, st::randompos_system);
}

Assets
load_assets(stlw::Logger &logger, opengl::OpenglPipelines &gfx)
{
  // LOAD different assets.
  //"assets/chalet.mtl"
  namespace OF = opengl::factories;

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
      {GL_TRIANGLES, objs.hashtag});

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

auto
make_tilemap(stlw::float_generator &rng)
{
  auto constexpr NUM_TILES = 1000;
  auto const [W, H, L] = std::array<std::size_t, 3>{10, 10, 10};
  assert((W * H * L) == NUM_TILES);

  auto tile_vec = std::vector<Tile>{};
  tile_vec.reserve(NUM_TILES);

  FOR(x, W) {
    FOR(y, H) {
      FOR(z, L) {
        Tile tile;
        tile.is_wall = rng.generate_bool();
        tile_vec.emplace_back(MOVE(tile));
      }
    }
  }

  assert(tile_vec.capacity() == tile_vec.size());
  assert(tile_vec.size() == NUM_TILES);
  return TileMap{MOVE(tile_vec), W, H, L};
}

template<typename PROXY>
auto
make_entities(PROXY &proxy)
{
  // Create entities
  std::vector<::opengl::Model*> entities;
  auto const make_entity = [&proxy, &entities](auto const& t) {
    auto eid = proxy.create_entity();
    auto &p = proxy.add_component(ct::model, eid);
    p.translation = t;

    entities.emplace_back(&p);
  };

  make_entity(glm::vec3{2.0f, 2.0f, 2.0f});   // COLOR_CUBE
  make_entity(glm::vec3{4.0f, -4.0f, -2.0f}); // TEXTURE_CUBE
  make_entity(glm::vec3{3.0f, -2.0f, 2.0f});  // WIREFRAME_CUBE_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // SKYBOX_INDEX
  make_entity(glm::vec3{2.0f, 0.0f, -4.0f});  // HOUSE_CUBE
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // AT_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // TILEMAP_INDEX
  make_entity(glm::vec3{0.0f, 5.0f, 0.0f});   // TERRAIN_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // CAMERA_INDEX

  //auto count = GameState::CAMERA_INDEX + 1;

  // The 2D objects
  /*
  while(count < (100 + GameState::CAMERA_INDEX)) {
    auto const [x, y, z] = rng.generate_3dposition_above_ground();
    make_entity(glm::vec3{x, y, z});
  }
  */
  return entities;
}

template<typename PROXY>
auto
init(stlw::Logger &logger, PROXY &proxy, ImGuiIO &imgui, window::Dimensions const& dimensions)
{
  auto const fheight = dimensions.h;
  auto const fwidth = dimensions.w;
  auto const aspect = static_cast<GLfloat>(fwidth / fheight);

  // Initialize opengl
  opengl::init(dimensions);

  // Configure Imgui
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  auto projection = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 200.0f);
  stlw::float_generator rng;

  auto tmap = make_tilemap(rng);
  auto entities = make_entities(proxy);

  return GameState(logger, imgui, dimensions, MOVE(rng), MOVE(projection), MOVE(tmap), MOVE(entities));
}

void game_loop(GameState &state, opengl::OpenglPipelines &gfx, Assets const& assets)
{
  opengl::clear_screen(opengl::LIST_OF_COLORS::LIGHT_BLUE);
  auto render_args = state.render_args();

  // skybox
  //opengl::draw(render_args, *state.entities[GameState::SKYBOX_INDEX], gfx.d3.skybox, assets.handles.cube_skybox);

  // random
  opengl::draw(render_args, *state.entities[GameState::COLOR_CUBE_INDEX], gfx.d3.color, assets.handles.cube_colored);
  opengl::draw(render_args, *state.entities[GameState::TEXTURE_CUBE_INDEX], gfx.d3.texture_cube, assets.handles.cube_textured);
  opengl::draw(render_args, *state.entities[GameState::WIREFRAME_CUBE_INDEX], gfx.d3.wireframe, assets.handles.cube_wireframe);

  // house
  opengl::draw(render_args, *state.entities[GameState::HOUSE_INDEX], gfx.d3.house, assets.handles.house);

  // tilemap
  opengl::draw_tilemap(render_args, *state.entities[GameState::TILEMAP_INDEX], gfx.d3.hashtag,
      assets.handles.tilemap, state.tilemap);


  // player
  opengl::draw(render_args, *state.entities[GameState::AT_INDEX], gfx.d3.at, assets.handles.at);

  // terrain
  //opengl::draw(render_args, *state.entities[GameState::TERRAIN_INDEX], gfx.d3.terrain, assets.handles.terrain);

  // UI code
  draw_ui(state);
}

} // ns boomhs
