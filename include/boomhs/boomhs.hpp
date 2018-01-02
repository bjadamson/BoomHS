#pragma once

#include <imgui/imgui.hpp>

#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <boomhs/skybox.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/level_generator.hpp>
#include <boomhs/randompos_system.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui.hpp>
#include <window/sdl_window.hpp>

#include <stlw/log.hpp>

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
  auto &d3 = gfx.d3;

  auto loader = ObjLoader{LOC::ORANGE};
  auto house_obj = loader.load_mesh("assets/house_uv.obj", LoadNormals{true}, LoadUvs{true});
  auto hashtag_obj = loader.load_mesh("assets/hashtag.obj", "assets/hashtag.mtl", LoadNormals{false}, LoadUvs{false});

  auto at_obj = loader.load_mesh("assets/at.obj", "assets/at.mtl", LoadNormals{false}, LoadUvs{false});
  auto plus_obj = loader.load_mesh("assets/plus.obj", "assets/plus.mtl", LoadNormals{false}, LoadUvs{false});

  loader.set_color(LOC::YELLOW);
  auto arrow_obj = loader.load_mesh("assets/arrow.obj", "assets/arrow.mtl", LoadNormals{false}, LoadUvs{false});

  Objs objs{MOVE(house_obj), MOVE(hashtag_obj), MOVE(at_obj), MOVE(plus_obj), MOVE(arrow_obj)};

  auto house_handle = OF::make_mesh(logger,   d3.house,   MeshProperties{objs.house});
  auto hashtag_handle = OF::make_mesh(logger, d3.hashtag, MeshProperties{objs.hashtag});
  auto at_handle = OF::make_mesh(logger,      d3.at,      MeshProperties{objs.at});
  auto plus_handle = OF::make_mesh(logger,    d3.plus,    MeshProperties{objs.plus});
  auto arrow_handle = OF::make_mesh(logger,    d3.arrow,  MeshProperties{objs.arrow});

  auto cube_skybox = OF::copy_cube_gpu(logger, d3.skybox, {{10.0f, 10.0f, 10.0f}});
  auto cube_textured = OF::copy_cube_gpu(logger, d3.texture_cube, {{0.15f, 0.15f, 0.15f}});

  auto cube_colored = OF::copy_cube_gpu(logger, d3.color, {{0.25f, 0.25f, 0.25f}},
      LOC::BLUE);

  auto cube_wf = OF::copy_cube_gpu(logger, d3.wireframe, {{0.25f, 0.25f, 0.25f}, true});

  auto terrain_handle = OF::copy_cube_gpu(logger, d3.terrain, {{2.0f, 0.4f, 2.0f}},
      LOC::SADDLE_BROWN);

  auto tilemap_handle = OF::copy_tilemap_gpu(logger, d3.hashtag,
      {GL_TRIANGLE_STRIP, objs.hashtag});

  auto world_arrows = OF::create_world_axis_arrows(logger, d3.global_x_axis_arrow,
      d3.global_y_axis_arrow, d3.global_z_axis_arrow);

  GpuHandles handles{
    MOVE(house_handle),
    MOVE(hashtag_handle),
    MOVE(at_handle),
    MOVE(plus_handle),
    MOVE(arrow_handle),

    MOVE(world_arrows.x_dinfo),
    MOVE(world_arrows.y_dinfo),
    MOVE(world_arrows.z_dinfo),

    MOVE(cube_skybox),
    MOVE(cube_textured),
    MOVE(cube_colored),
    MOVE(cube_wf),
    MOVE(terrain_handle),
    MOVE(tilemap_handle)};
  return Assets{MOVE(objs), MOVE(handles)};
}

auto
tiles_in_line(int x0, int y0, int x1, int y1)
{
  std::vector<TilePosition> positions;

  int dx = abs(x1-x0);
  int dy = abs(y1-y0);

  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx-dy;

  while (true) {
    positions.emplace_back(TilePosition{x0, y0, 0});

    if (x0==x1 && y0==y1) {
      break;
    }

    int e2 = err * 2;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx){
      err += dx;
      y0 += sy;
    }
  }
  return positions;
}

void
bresenham_3d(int x0, int y0, int z0, int x1, int y1, int z1, TileMap &tmap)
{
  int const dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int const dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int const dz = abs(z1-z0), sz = z0<z1 ? 1 : -1;
  auto const arr = stlw::make_array<int>(dx, dy, dz);

  //int const dm = std::max(dx,dy,dz); /* maximum difference */
  auto const it = std::max_element(arr.cbegin(), arr.cend());
  assert(it);
  int const dm = *it;
  int i = dm;
  x1 = y1 = z1 = dm/2; /* error offset */

  bool found_wall = false;
  auto const set_tile = [&found_wall](auto &tile) {
    if (found_wall) {
      // Can't see tile's behind a wall.
      tile.is_visible = false;
    }
    else if (!tile.is_wall) {
      tile.is_visible = true;
    } else if (tile.is_wall) {
      found_wall = true;
      tile.is_visible = true;
    } else {
      tile.is_visible = false;
    }
  };

   for(;;) {  /* loop */
     auto &tile = tmap.data(x0, y0, z0);
      set_tile(tile);
      if (i-- == 0) break;
      x1 -= dx; if (x1 < 0) { x1 += dm; x0 += sx; }
      y1 -= dy; if (y1 < 0) { y1 += dm; y0 += sy; }
      z1 -= dz; if (z1 < 0) { z1 += dm; z0 += sz; }
   }
}

void
update_visible_tiles(TileMap &tmap, Player const& player, bool const reveal_tilemap)
{
  auto const& wp = player.world_position();

  // Collect all the visible tiles for the player
  auto const [w, h, l] = tmap.dimensions();

  std::vector<TilePosition> visited;
  auto const update_tile = [&tmap, &visited](TilePosition const& pos) {
    bool found_wall = false;
      auto &tile = tmap.data(pos.x, pos.y, pos.z);
      if (!found_wall && !tile.is_wall) {
        // This is probably not always necessary. Consider starting with all tiles visible?
        tile.is_visible = true;
      }
      else if(!found_wall && tile.is_wall) {
        tile.is_visible = true;
        found_wall = true;
      } else {
        tile.is_visible = false;
      }
    };

  std::vector<TilePosition> positions;
  FOR(x, w) {
    FOR(y, h) {
      FOR (z, l) {
        if (reveal_tilemap) {
          tmap.data(x, y, z).is_visible = true;
        } else {
          bresenham_3d(wp.x, wp.y, wp.z, x, y, z, tmap);
        }
      }
    }
  }
  for (auto const& pos : positions) {
    auto const cmp = [&pos](auto const& pcached) {
      return pcached == pos;
    };
    bool const seen_already = std::find_if(visited.cbegin(), visited.cend(), cmp) != visited.cend();
    if (seen_already) {
      // This tile has already been visited, skip
      continue;
    }
    visited.emplace_back(pos);
  }
  for (auto const& ppos : visited) {
    update_tile(ppos);
  }
}

template<typename PROXY>
auto
make_entities(PROXY &proxy)
{
  // Create entities
  std::vector<Transform*> entities;
  auto const make_entity = [&proxy, &entities](auto const& t) {
    auto eid = proxy.create_entity();
    auto &p = proxy.add_component(ct::transform, eid);
    p.translation = t;

    entities.emplace_back(&p);
  };

  glm::vec3 constexpr ZERO = glm::zero<glm::vec3>();
  glm::vec3 constexpr TILEMAP_POS = ZERO;
  make_entity(glm::vec3{-2.0f, -2.0f, -2.0f}); // COLOR_CUBE
  make_entity(glm::vec3{-4.0f, -4.0f, -2.0f}); // TEXTURE_CUBE
  make_entity(glm::vec3{-3.0f, -2.0f, 2.0f});  // WIREFRAME_CUBE_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});    // SKYBOX_INDEX
  make_entity(glm::vec3{2.0f, 0.0f, -4.0f});   // HOUSE_CUBE
  make_entity(ZERO);                           // AT_INDEX
  make_entity(ZERO);                           // PLAYER_ARROW_INDEX
  make_entity(TILEMAP_POS);                    // TILEMAP_INDEX
  make_entity(glm::vec3{0.0f, 5.0f, 0.0f});    // TERRAIN_INDEX
  make_entity(ZERO);                           // CAMERA_INDEX

  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});    // GLOBAL_AXIS_X_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});    // GLOBAL_AXIS_Y_INDEX
  make_entity(glm::vec3{0.0f, 0.0f, 0.0f});    // GLOBAL_AXIS_Z_INDEX

  make_entity(ZERO);                           //LOCAL_AXIS_X_INDEX
  make_entity(ZERO);                           //LOCAL_AXIS_Y_INDEX
  make_entity(ZERO);                           //LOCAL_AXIS_Z_INDEX
  make_entity(ZERO);                           //LOCAL_FORWARD_INDEX

  make_entity(ZERO);                           //CAMERA_ARROW_INDEX0
  make_entity(ZERO);                           //CAMERA_ARROW_INDEX1
  make_entity(ZERO);                           //CAMERA_ARROW_INDEX2
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
  render::init(dimensions);

  // Configure Imgui
  imgui.MouseDrawCursor = true;
  imgui.DisplaySize = ImVec2{static_cast<float>(dimensions.w), static_cast<float>(dimensions.h)};

  Projection const proj{90.0f, 4.0f/3.0f, 0.1f, 200.0f};

  stlw::float_generator rng;
  auto tmap_startingpos = level_generator::make_tilemap(80, 1, 45, rng);
  auto tmap = MOVE(tmap_startingpos.first);
  auto entities = make_entities(proxy);

  auto &skybox_ent = *entities[GameState::SKYBOX_INDEX];
  auto &player_ent = *entities[GameState::AT_INDEX];
  player_ent.rotation = glm::angleAxis(glm::radians(180.0f), opengl::Y_UNIT_VECTOR);
  player_ent.scale = 1.0f * glm::one<glm::vec3>();

  auto &arrow_ent = *entities[GameState::PLAYER_ARROW_INDEX];
  arrow_ent.scale = glm::vec3{0.025f, 0.025f, 0.025f};
  arrow_ent.translation = arrow_ent.translation + (0.125f * opengl::Y_UNIT_VECTOR);

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -opengl::Z_UNIT_VECTOR;
  auto constexpr UP = opengl::Y_UNIT_VECTOR;
  Player player{player_ent, arrow_ent, FORWARD, UP};
  {
    auto &startingpos = tmap_startingpos.second;
    auto const pos = glm::vec3{startingpos.x, startingpos.y, startingpos.z};
    std::cerr << "starting pos: '" << glm::to_string(pos) << "'\n";
    player.move_to(pos);
  }
  Camera camera(proj, player_ent, FORWARD, UP);

  return GameState{logger, imgui, dimensions, MOVE(rng), MOVE(tmap), MOVE(entities), MOVE(camera),
      MOVE(player), MOVE(Skybox{skybox_ent})};
}

template<typename PROXY>
void game_loop(GameState &state, PROXY &proxy, opengl::OpenglPipelines &gfx, window::SDLWindow &window,
    Assets &assets)
{
  using GS = GameState;
  auto rargs = state.render_args();
  auto const& ents = state.entities;
  auto const& handles = assets.handles;
  auto &player = state.player;
  auto &camera = state.camera;
  auto &d3 = gfx.d3;
  auto &logger = state.logger;

  if (state.render.tilemap.redraw) {
    std::cerr << "Updating tilemap\n";
    update_visible_tiles(state.tilemap, state.player, state.render.tilemap.reveal);
    //assets.handles.tilemap = OF::copy_tilemap_gpu(logger, d3.hashtag,
        //{GL_TRIANGLE_STRIP, assets.objects.hashtag});

    // We don't need to recompute the tilemap, we just did.
    state.render.tilemap.redraw = false;
  }

  render::clear_screen(LOC::BLACK);

  // skybox
  state.skybox.transform.translation = ents[GameState::AT_INDEX]->translation;
  if (state.render.draw_skybox) {
    render::draw(rargs, state.skybox.transform, d3.skybox, handles.cube_skybox);
  }

  // random
  render::draw(rargs, *ents[GS::COLOR_CUBE_INDEX], d3.color, handles.cube_colored);
  render::draw(rargs, *ents[GS::TEXTURE_CUBE_INDEX], d3.texture_cube, handles.cube_textured);
  render::draw(rargs, *ents[GS::WIREFRAME_CUBE_INDEX], d3.wireframe, handles.cube_wireframe);

  // house
  render::draw(rargs, *ents[GS::HOUSE_INDEX], d3.house, handles.house);

  // tilemap
  render::draw_tilemap(rargs, *ents[GS::TILEMAP_INDEX],
      {handles.hashtag, d3.hashtag, handles.plus, d3.plus},
      state.tilemap, state.render.tilemap.reveal);

  if (state.render.tilemap.show_grid_lines) {
    auto &pipeline = d3.global_x_axis_arrow;
    auto const tilegrid = OF::create_tilegrid(logger, pipeline, state.tilemap,
        state.render.tilemap.show_yaxis_lines);
    render::draw_tilegrid(rargs, *ents[GS::TILEMAP_INDEX], pipeline, tilegrid);
  }

  // player
  render::draw(rargs, *ents[GS::AT_INDEX], d3.at, handles.at);

  // arrow
  render::draw(rargs, *ents[GS::PLAYER_ARROW_INDEX], d3.arrow, handles.arrow);

  // global coordinates
  if (state.render.show_global_axis) {
    render::draw(rargs, *ents[GS::GLOBAL_AXIS_X_INDEX], d3.global_x_axis_arrow, handles.x_axis_arrow);
    render::draw(rargs, *ents[GS::GLOBAL_AXIS_Y_INDEX], d3.global_y_axis_arrow, handles.y_axis_arrow);
    render::draw(rargs, *ents[GS::GLOBAL_AXIS_Z_INDEX], d3.global_z_axis_arrow, handles.z_axis_arrow);
  }

  // local coordinates
  if (state.render.show_local_axis) {
    auto const& player_pos = ents[GS::AT_INDEX]->translation;
    {
      auto const world_coords = OF::create_axis_arrows(logger,
          d3.local_x_axis_arrow,
          d3.local_y_axis_arrow,
          d3.local_z_axis_arrow,
          player_pos);
      render::draw(rargs, *ents[GS::LOCAL_AXIS_X_INDEX], d3.local_x_axis_arrow, world_coords.x_dinfo);
      render::draw(rargs, *ents[GS::LOCAL_AXIS_Y_INDEX], d3.local_y_axis_arrow, world_coords.y_dinfo);
      render::draw(rargs, *ents[GS::LOCAL_AXIS_Z_INDEX], d3.local_z_axis_arrow, world_coords.z_dinfo);
    }
  }

  // draw forward arrow (for player)
  if (state.render.show_target_vectors) {
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + (1.0f * player.forward_vector());

      auto const handle = OF::create_arrow(logger, gfx.d3.local_forward_arrow,
          OF::ArrowCreateParams{LOC::LIGHT_BLUE, start, head});

      render::draw(rargs, *ents[GS::LOCAL_FORWARD_INDEX], d3.local_forward_arrow, handle);
    }
    // draw forward arrow (for camera)
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = camera.world_position();
      auto handle = OF::create_arrow(logger, gfx.d3.camera_arrow0,
        OF::ArrowCreateParams{LOC::YELLOW, start, head});

      render::draw(rargs, *ents[GS::CAMERA_ARROW_INDEX0], d3.camera_arrow0, handle);
    }
    // draw forward arrow (for camera)
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.backward_vector();

      auto const handle = OF::create_arrow(logger, gfx.d3.camera_arrow1,
        OF::ArrowCreateParams{LOC::PINK, start, head});

      render::draw(rargs, *ents[GS::CAMERA_ARROW_INDEX1], d3.camera_arrow1, handle);
    }
    // draw arrow from origin -> camera
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.right_vector();

      auto const handle = OF::create_arrow(logger, gfx.d3.camera_arrow2,
        OF::ArrowCreateParams{LOC::PURPLE, start, head});

      render::draw(rargs, *ents[GS::CAMERA_ARROW_INDEX2], d3.camera_arrow2, handle);
    }
  }

  // terrain
  //render::draw(rargs, *ents[GS::TERRAIN_INDEX], d3.terrain, handles.terrain);

  // UI code
  draw_ui(state, window, proxy);
}

} // ns boomhs
