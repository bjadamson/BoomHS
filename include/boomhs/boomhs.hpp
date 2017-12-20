#pragma once

#include <imgui/imgui.hpp>

#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <opengl/renderer.hpp>
#include <opengl/skybox.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/randompos_system.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui.hpp>

#include <stlw/log.hpp>

#include <vector>

using stlw::Logger;
namespace OF = opengl::factories;
namespace LOC = opengl::LIST_OF_COLORS;

namespace boomhs
{

Assets
load_assets(stlw::Logger &logger, opengl::OpenglPipelines &gfx)
{
  // LOAD different assets.
  //"assets/chalet.mtl"
  using namespace opengl;

  auto loader = ObjLoader{LOC::ORANGE};
  auto house_obj = loader.load_mesh("assets/house_uv.obj", LoadNormals{true}, LoadUvs{true});
  auto hashtag_obj = loader.load_mesh("assets/hashtag.obj", "assets/hashtag.mtl", LoadNormals{false}, LoadUvs{false});

  auto at_obj = loader.load_mesh("assets/at.obj", "assets/at.mtl", LoadNormals{false}, LoadUvs{false});
  auto plus_obj = loader.load_mesh("assets/plus.obj", "assets/plus.mtl", LoadNormals{false}, LoadUvs{false});

  loader.set_color(LOC::YELLOW);
  auto arrow_obj = loader.load_mesh("assets/arrow.obj", "assets/arrow.mtl", LoadNormals{false}, LoadUvs{false});

  //loader.set_color(LOC::RED);
  //auto x_arrow_obj = loader.load_mesh("assets/arrow.obj", "assets/arrow.mtl", LoadNormals{false}, LoadUvs{false});

  //loader.set_color(LOC::GREEN);
  //auto y_arrow_obj = loader.load_mesh("assets/arrow.obj", "assets/arrow.mtl", LoadNormals{false}, LoadUvs{false});

  //loader.set_color(LOC::BLUE);
  //auto z_arrow_obj = loader.load_mesh("assets/arrow.obj", "assets/arrow.mtl", LoadNormals{false}, LoadUvs{false});

  Objs objs{MOVE(house_obj), MOVE(hashtag_obj), MOVE(at_obj), MOVE(plus_obj), MOVE(arrow_obj)};
      //MOVE(x_arrow_obj), MOVE(y_arrow_obj), MOVE(z_arrow_obj)};

  auto house_handle = OF::make_mesh(logger,   gfx.d3.house,   MeshProperties{objs.house});
  auto hashtag_handle = OF::make_mesh(logger, gfx.d3.hashtag, MeshProperties{objs.hashtag});
  auto at_handle = OF::make_mesh(logger,      gfx.d3.at,      MeshProperties{objs.at});
  auto plus_handle = OF::make_mesh(logger,    gfx.d3.plus,    MeshProperties{objs.plus});
  auto arrow_handle = OF::make_mesh(logger,    gfx.d3.arrow,  MeshProperties{objs.arrow});

  auto cube_skybox = OF::copy_cube_gpu(logger, gfx.d3.skybox, {{10.0f, 10.0f, 10.0f}});
  auto cube_textured = OF::copy_cube_gpu(logger, gfx.d3.texture_cube, {{0.15f, 0.15f, 0.15f}});

  auto cube_colored = OF::copy_cube_gpu(logger, gfx.d3.color, {{0.25f, 0.25f, 0.25f}},
      LOC::BLUE);

  auto cube_wf = OF::copy_cube_gpu(logger, gfx.d3.wireframe, {{0.25f, 0.25f, 0.25f}, true});

  auto terrain_handle = OF::copy_cube_gpu(logger, gfx.d3.terrain, {{2.0f, 0.4f, 2.0f}},
      LOC::SADDLE_BROWN);

  auto tilemap_handle = OF::copy_tilemap_gpu(logger, gfx.d3.hashtag,
      {GL_TRIANGLE_STRIP, objs.hashtag});

  glm::vec3 constexpr ORIGIN{0.0f, 0.0f, 0.0f};
  auto x_axis_handle = OF::create_x_axis_arrow(logger, gfx.d3.global_x_axis_arrow, ORIGIN);
  auto y_axis_handle = OF::create_y_axis_arrow(logger, gfx.d3.global_y_axis_arrow, ORIGIN);
  auto z_axis_handle = OF::create_z_axis_arrow(logger, gfx.d3.global_z_axis_arrow, ORIGIN);

  GpuHandles handles{
    MOVE(house_handle),
    MOVE(hashtag_handle),
    MOVE(at_handle),
    MOVE(plus_handle),
    MOVE(arrow_handle),

    MOVE(x_axis_handle),
    MOVE(y_axis_handle),
    MOVE(z_axis_handle),

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
  auto const [W, H, L] = stlw::make_array<std::size_t>(10ul, 1ul, 10ul);
  auto const NUM_TILES = W * H * L;

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

  // origin is never a wall
  if (!tile_vec.empty()) {
    Tile &origin = tile_vec.front();
    origin.is_wall = false;
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

  glm::vec3 constexpr TILEMAP_POS = glm::vec3{0.2f, 0.0f, 0.2f};
  make_entity(glm::vec3{-2.0f, -2.0f, -2.0f});   // COLOR_CUBE
  make_entity(glm::vec3{-4.0f, -4.0f, -2.0f}); // TEXTURE_CUBE
  make_entity(glm::vec3{-3.0f, -2.0f, 2.0f}); // WIREFRAME_CUBE_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // SKYBOX_INDEX
  make_entity(glm::vec3{2.0f, 0.0f, -4.0f});  // HOUSE_CUBE
  make_entity(TILEMAP_POS);                   // AT_INDEX
  make_entity(TILEMAP_POS);                   // PLAYER_ARROW_INDEX
  make_entity(TILEMAP_POS);                   // TILEMAP_INDEX
  make_entity(glm::vec3{0.0f, 5.0f, 0.0f});   // TERRAIN_INDEX
  make_entity(TILEMAP_POS);                   // CAMERA_INDEX

  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // GLOBAL_AXIS_X_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // GLOBAL_AXIS_Y_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});   // GLOBAL_AXIS_Z_INDEX

  make_entity(glm::vec3{TILEMAP_POS});        //LOCAL_AXIS_X_INDEX
  make_entity(glm::vec3{TILEMAP_POS});        //LOCAL_AXIS_Y_INDEX
  make_entity(glm::vec3{TILEMAP_POS});        //LOCAL_AXIS_Z_INDEX
  make_entity(glm::vec3{TILEMAP_POS});        //LOCAL_FORWARD_INDEX
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

  using CF = opengl::CameraFactory;
  opengl::Projection const proj{90.0f, 4.0f/3.0f, 0.1f, 200.0f};

  stlw::float_generator rng;
  auto tmap = make_tilemap(rng);
  auto entities = make_entities(proxy);

  auto &skybox_ent = *entities[GameState::SKYBOX_INDEX];
  auto &player_ent = *entities[GameState::AT_INDEX];
  player_ent.scale = glm::vec3{0.25f};

  auto &arrow_ent = *entities[GameState::PLAYER_ARROW_INDEX];
  arrow_ent.scale = glm::vec3{0.1f, 0.1f, 0.1f};

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.  auto constexpr UP = opengl::Y_UNIT_VECTOR;
  auto const FORWARD = -opengl::Z_UNIT_VECTOR;
  auto constexpr UP = opengl::Y_UNIT_VECTOR;
  auto camera = CF::make_default(proj, skybox_ent, player_ent, FORWARD, UP);
  Player player{player_ent, arrow_ent, FORWARD, UP};

  return GameState{logger, imgui, dimensions, MOVE(rng), MOVE(tmap), MOVE(entities), MOVE(camera),
      MOVE(player)};
}

template<typename PROXY>
void game_loop(GameState &state, PROXY &proxy, opengl::OpenglPipelines &gfx, Assets const& assets)
{
  opengl::clear_screen(LOC::BLACK);
  auto rargs = state.render_args();
  auto const& ents = state.entities;
  auto const& handles = assets.handles;
  auto &d3 = gfx.d3;

  using GS = GameState;

  // skybox
  if (state.draw_skybox) {
    opengl::draw(rargs, *ents[GameState::SKYBOX_INDEX], d3.skybox, handles.cube_skybox);
  }

  // random
  opengl::draw(rargs, *ents[GS::COLOR_CUBE_INDEX], d3.color, handles.cube_colored);
  opengl::draw(rargs, *ents[GS::TEXTURE_CUBE_INDEX], d3.texture_cube, handles.cube_textured);
  opengl::draw(rargs, *ents[GS::WIREFRAME_CUBE_INDEX], d3.wireframe, handles.cube_wireframe);

  // house
  opengl::draw(rargs, *ents[GS::HOUSE_INDEX], d3.house, handles.house);

  // tilemap
  opengl::draw_tilemap(rargs, *ents[GS::TILEMAP_INDEX],
      {handles.hashtag, d3.hashtag, handles.plus, d3.plus},
      state.tilemap);

  // player
  opengl::draw(rargs, *ents[GS::AT_INDEX], d3.at, handles.at);

  // arrow
  opengl::draw(rargs, *ents[GS::PLAYER_ARROW_INDEX], d3.arrow, handles.arrow);

  // global coordinates
  opengl::draw(rargs, *ents[GS::GLOBAL_AXIS_X_INDEX], d3.global_x_axis_arrow, handles.x_axis_arrow);
  opengl::draw(rargs, *ents[GS::GLOBAL_AXIS_Y_INDEX], d3.global_y_axis_arrow, handles.y_axis_arrow);
  opengl::draw(rargs, *ents[GS::GLOBAL_AXIS_Z_INDEX], d3.global_z_axis_arrow, handles.z_axis_arrow);

  // local coordinates
  float constexpr lscale = 0.95f;
  auto const& player_pos = ents[GS::AT_INDEX]->translation;
  {
    auto handle = OF::create_x_axis_arrow(state.logger, gfx.d3.local_x_axis_arrow, player_pos, lscale);
    opengl::draw(rargs, *ents[GS::LOCAL_AXIS_X_INDEX], d3.local_x_axis_arrow, handle);
  }
  {
    auto handle = OF::create_y_axis_arrow(state.logger, gfx.d3.local_y_axis_arrow, player_pos, lscale);
    opengl::draw(rargs, *ents[GS::LOCAL_AXIS_Y_INDEX], d3.local_y_axis_arrow, handle);
  }
  {
    auto handle = OF::create_z_axis_arrow(state.logger, gfx.d3.local_z_axis_arrow, player_pos, lscale);
    opengl::draw(rargs, *ents[GS::LOCAL_AXIS_Z_INDEX], d3.local_z_axis_arrow, handle);
  }
  // draw forward line
  {
    glm::vec3 const begin = state.player.position();
    glm::vec3 const end = state.player.position() + (1.0f * state.player.forward_vector());
    auto handle = OF::create_line(state.logger, gfx.d3.local_forward_arrow,
        begin, LOC::BROWN, end, LOC::PINK);

    opengl::draw(rargs, *ents[GS::LOCAL_FORWARD_INDEX],
        d3.local_forward_arrow, handle);
  }

  // terrain
  //opengl::draw(rargs, *ents[GS::TERRAIN_INDEX], d3.terrain, handles.terrain);

  // UI code
  draw_ui(state, proxy);
}

} // ns boomhs
