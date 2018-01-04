#pragma once

#include <imgui/imgui.hpp>

#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <boomhs/skybox.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/level_generator.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui.hpp>
#include <window/sdl_window.hpp>

#include <stlw/log.hpp>

using stlw::Logger;
using GS = boomhs::GameState;
namespace OF = opengl::factories;
namespace LOC = opengl::LIST_OF_COLORS;

namespace boomhs
{

Assets
load_assets(stlw::Logger &logger, opengl::ShaderPrograms &gfx)
{
  // LOAD different assets.
  //"assets/chalet.mtl"
  using namespace opengl;
  auto &d3 = gfx.d3;

  auto loader = ObjLoader{LOC::WHITE};
  auto house_obj = loader.load_mesh("assets/house_uv.obj", LoadNormals{true}, LoadUvs{true});
  auto hashtag_obj = loader.load_mesh("assets/hashtag.obj", "assets/hashtag.mtl", LoadNormals{true}, LoadUvs{false});

  auto at_obj = loader.load_mesh("assets/at.obj", "assets/at.mtl", LoadNormals{true}, LoadUvs{false});
  loader.set_color(LOC::ORANGE);
  auto plus_obj = loader.load_mesh("assets/plus.obj", "assets/plus.mtl", LoadNormals{true}, LoadUvs{false});

  loader.set_color(LOC::PINK);
  auto O_obj = loader.load_mesh("assets/O.obj", "assets/O.mtl", LoadNormals{true}, LoadUvs{false});
  loader.set_color(LOC::PURPLE);
  auto T_obj = loader.load_mesh("assets/T.obj", "assets/T.mtl", LoadNormals{true}, LoadUvs{false});

  loader.set_color(LOC::YELLOW);
  auto arrow_obj = loader.load_mesh("assets/arrow.obj", "assets/arrow.mtl", LoadNormals{true}, LoadUvs{false});

  Objs objs{MOVE(house_obj), MOVE(hashtag_obj), MOVE(at_obj), MOVE(plus_obj), MOVE(arrow_obj),
    MOVE(O_obj), MOVE(T_obj)};

  auto house_handle = OF::copy_gpu(logger,   GL_TRIANGLES, d3.house,   objs.house);
  auto hashtag_handle = OF::copy_gpu(logger, GL_TRIANGLES, d3.hashtag, objs.hashtag);
  auto at_handle = OF::copy_gpu(logger,      GL_TRIANGLES, d3.at,      objs.at);
  auto plus_handle = OF::copy_gpu(logger,    GL_TRIANGLES, d3.plus,    objs.plus);
  auto arrow_handle = OF::copy_gpu(logger,    GL_TRIANGLES, d3.arrow,  objs.arrow);

  auto const make_letter = [&](auto const& pipe, auto const& obj) {
    return OF::copy_gpu(logger, GL_TRIANGLES, pipe, obj);
  };

  auto O_handle = make_letter(d3.O, objs.O);
  auto T_handle = make_letter(d3.T, objs.T);

  auto cube_skybox = OF::copy_texturecube_gpu(logger, d3.skybox);
  auto cube_textured = OF::copy_texturecube_gpu(logger, d3.texture_cube);

  auto cube_colored = OF::copy_colorcube_gpu(logger, d3.color, LOC::BLUE);
  auto terrain_handle = OF::copy_colorcube_gpu(logger, d3.terrain, LOC::SADDLE_BROWN);

  auto tilemap_handle = OF::copy_gpu(logger, GL_TRIANGLE_STRIP, d3.hashtag, objs.hashtag);

  auto world_arrows = OF::create_world_axis_arrows(logger, d3.global_x_axis_arrow,
      d3.global_y_axis_arrow, d3.global_z_axis_arrow);

  GpuHandles handles{
    MOVE(house_handle),
    MOVE(hashtag_handle),
    MOVE(at_handle),
    MOVE(plus_handle),
    MOVE(arrow_handle),

    MOVE(O_handle),
    MOVE(T_handle),

    MOVE(world_arrows.x_dinfo),
    MOVE(world_arrows.y_dinfo),
    MOVE(world_arrows.z_dinfo),

    MOVE(cube_skybox),
    MOVE(cube_textured),
    MOVE(cube_colored),
    MOVE(terrain_handle),
    MOVE(tilemap_handle)};
  return Assets{MOVE(objs), MOVE(handles)};
}



template<typename PROXY>
auto
make_entities(PROXY &proxy)
{
  // Create entities
  std::vector<Transform*> entities;
  auto const make_entity = [&proxy, &entities]() {
    auto eid = proxy.create_entity();
    auto &p = proxy.add_component(ct::transform, eid);
    p.translation = glm::zero<glm::vec3>();

    entities.emplace_back(&p);
  };

  FOR(i, GameState::INDEX_MAX) {
    make_entity();
  }

  entities[GS::COLOR_CUBE_INDEX]->translation = glm::vec3{-2.0f, -2.0f, -2.0f};
  entities[GS::TEXTURE_CUBE_INDEX]->translation = glm::vec3{-4.0f, -4.0f, -2.0f};
  entities[GS::WIREFRAME_CUBE_INDEX]->translation = glm::vec3{-3.0f, -2.0f, 2.0f};
  entities[GS::SKYBOX_INDEX]->translation = glm::vec3{0.0f, 0.0f, 0.0f};
  entities[GS::HOUSE_INDEX]->translation = glm::vec3{2.0f, 0.0f, -4.0f};
  entities[GS::TERRAIN_INDEX]->translation = glm::vec3{0.0f, 5.0f, 0.0f};

  //auto const make_standing = [&entities](int const index) {
    //auto &entity = *entities[index];
    //entity.translation = glm::vec3{0.0f, 0.0f, 0.0f};
    //entity.rotation = glm::rotate(glm::mat4{}, glm::radians(90.0f), opengl::X_UNIT_VECTOR);
  //};

  //make_standing(GS::ORC_INDEX);
  //make_standing(GS::TROLL_INDEX);

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

  auto &skybox_ent = *entities[GS::SKYBOX_INDEX];
  auto &player_ent = *entities[GS::AT_INDEX];
  player_ent.rotation = glm::angleAxis(glm::radians(180.0f), opengl::Y_UNIT_VECTOR);

  auto &arrow_ent = *entities[GS::PLAYER_ARROW_INDEX];
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
    player.move_to(pos);

    auto &light_ent = *entities[GS::LIGHT_INDEX];
    light_ent.translation = pos;
    //light_ent.translation.y += 1.0f;
    //light_ent.translation.z += 5.0f;
  }
  Camera camera(proj, player_ent, FORWARD, UP);

  GameState gs{logger, imgui, dimensions, MOVE(rng), MOVE(tmap), MOVE(entities), MOVE(camera),
      MOVE(player), MOVE(Skybox{skybox_ent})};
  return MOVE(gs);
}

template<typename PROXY>
void game_loop(GameState &state, PROXY &proxy, opengl::ShaderPrograms &gfx, window::SDLWindow &window,
    Assets &assets)
{
  auto &player = state.player;
  auto &mouse = state.mouse;
  auto &render = state.render;

  // game logic
  if (mouse.right_pressed && mouse.left_pressed) {
    player.move(0.25f, player.forward_vector());
    render.tilemap.redraw = true;
  }

  auto &logger = state.logger;
  auto &camera = state.camera;
  auto rargs = state.render_args();
  auto const& ents = state.entities;
  auto const& handles = assets.handles;

  ///////////////////////////////////////////////////////////////////
  // drawing
  if (render.tilemap.redraw) {
    std::cerr << "Updating tilemap\n";
    update_visible_tiles(state.tilemap, player, render.tilemap.reveal);

    // We don't need to recompute the tilemap, we just did.
    state.render.tilemap.redraw = false;
  }

  auto &d3 = gfx.d3;
  render::clear_screen(LOC::BLACK);

  // light
  {
    auto light_handle = OF::copy_colorcube_gpu(logger, d3.light0, state.light.diffuse);
    render::draw(rargs, *ents[GS::LIGHT_INDEX], d3.light0, light_handle);
  }

  // skybox
  state.skybox.transform.translation = ents[GameState::AT_INDEX]->translation;
  if (state.render.draw_skybox) {
    render::draw(rargs, state.skybox.transform, d3.skybox, handles.cube_skybox);
  }

  // random
  render::draw(rargs, *ents[GS::COLOR_CUBE_INDEX], d3.color, handles.cube_colored);
  render::draw(rargs, *ents[GS::TEXTURE_CUBE_INDEX], d3.texture_cube, handles.cube_textured);
  //render::draw(rargs, *ents[GS::WIREFRAME_CUBE_INDEX], d3.wireframe, handles.cube_wireframe);

  // house
  render::draw(rargs, *ents[GS::HOUSE_INDEX], d3.house, handles.house);

  // tilemap
  render::draw_tilemap(rargs, *ents[GS::TILEMAP_INDEX],
      {handles.hashtag, d3.hashtag, handles.plus, d3.plus},
      state.tilemap, state.render.tilemap.reveal);

  if (state.render.tilemap.show_grid_lines) {
    auto &sp = d3.global_x_axis_arrow;
    auto const tilegrid = OF::create_tilegrid(logger, sp, state.tilemap,
        state.render.tilemap.show_yaxis_lines);
    render::draw_tilegrid(rargs, *ents[GS::TILEMAP_INDEX], {sp, tilegrid});
  }

  // player
  render::draw(rargs, *ents[GS::AT_INDEX], d3.at, handles.at);

  // enemies
  render::draw(rargs, *ents[GS::ORC_INDEX], d3.O, handles.O);
  render::draw(rargs, *ents[GS::TROLL_INDEX], d3.T, handles.T);

  // arrow
  //render::draw(rargs, *ents[GS::PLAYER_ARROW_INDEX], d3.arrow, handles.arrow);

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
      glm::vec3 const head = start + (2.0f * player.forward_vector());

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
  draw_ui(state, window);
}

} // ns boomhs
