#pragma once
#include <cassert>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <imgui/imgui.hpp>

#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/assets.hpp>
#include <boomhs/level_generator.hpp>
#include <boomhs/renderer.hpp>
#include <boomhs/tilemap.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>
#include <boomhs/ui.hpp>
#include <window/sdl_window.hpp>

#include <stlw/log.hpp>

using stlw::Logger;

namespace OF = opengl::factories;
namespace LOC = opengl::LIST_OF_COLORS;

namespace boomhs
{

stlw::result<DrawHandles, std::string>
copy_assets_gpu(stlw::Logger &logger, ObjCache const& obj_cache, opengl::ShaderPrograms &sps,
    opengl::TextureTable const& ttable)
{
  using namespace opengl;

  GpuHandles handles;
  auto const handle_set = [&](auto const mode, char const* handle_name,
      char const* shadername, char const* obj_name, char const* texture_name)
  {
    auto const& obj = obj_cache.get_obj(obj_name);
    auto texture_o = ttable.lookup_texture(texture_name);

    auto &sp = sps.ref_sp(shadername);
    auto handle = OF::copy_gpu(logger, mode, sp, obj, MOVE(texture_o));
    handles.set(handle_name, MOVE(handle));
  };

  // clang-format off
  //         DRAW_MODE          HANDLE_NAME  SHADERNAME             OBJFILENAME  TEXTURENAME
  handle_set(GL_TRIANGLES,      HOUSE,       "3dtexture",           "house",    "TextureAtlas");
  handle_set(GL_TRIANGLE_STRIP, TILEMAP,     "hashtag",             "hashtag",  nullptr);

  handle_set(GL_TRIANGLES,      HASHTAG,     "hashtag",             "hashtag",  nullptr);
  handle_set(GL_TRIANGLES,      AT,          "3d_pos_normal_color", "at",       nullptr);
  handle_set(GL_TRIANGLES,      PLUS,        "plus",                "plus",     nullptr);
  handle_set(GL_TRIANGLES,      ORC,         "3d_pos_normal_color", "O",        nullptr);
  handle_set(GL_TRIANGLES,      TROLL,       "3d_pos_normal_color", "T",        nullptr);
  // clang-format on

  auto const copy_texturecube = [&logger, &sps, &ttable](char const* shadername, char const* texture_name) {
    auto texture_o = ttable.lookup_texture(texture_name);
    return OF::copy_texturecube_gpu(logger, sps.ref_sp(shadername), MOVE(texture_o));
  };

  handles.set(SKYBOX,       copy_texturecube("skybox", "skybox"));
  handles.set(TEXTURE_CUBE, copy_texturecube("3dcube_texture", "cube"));

  auto const copy_cubecolor = [&logger](auto const& shader, opengl::Color const color) {
    return OF::copy_colorcube_gpu(logger, shader, color);
  };
  handles.set(TERRAIN, copy_cubecolor(sps.ref_sp("3d_pos_color"), LOC::SADDLE_BROWN));

  // world-axis
  auto world_arrows = OF::create_world_axis_arrows(logger, sps.ref_sp("3d_pos_color"),
      sps.ref_sp("3d_pos_color"), sps.ref_sp("3d_pos_color"));

  handles.set(GLOBAL_AXIS_X, MOVE(world_arrows.x_dinfo));
  handles.set(GLOBAL_AXIS_Y, MOVE(world_arrows.y_dinfo));
  handles.set(GLOBAL_AXIS_Z, MOVE(world_arrows.z_dinfo));
  return DrawHandles{MOVE(handles)};
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

  FOR(i, INDEX_MAX) {
    make_entity();
  }

  entities[TEXTURE_CUBE_INDEX]->translation = glm::vec3{-4.0f, -4.0f, -2.0f};
  entities[SKYBOX_INDEX]->translation = glm::vec3{0.0f, 0.0f, 0.0f};
  entities[HOUSE_INDEX]->translation = glm::vec3{2.0f, 0.0f, -4.0f};
  entities[TERRAIN_INDEX]->translation = glm::vec3{0.0f, 5.0f, 0.0f};
  return entities;
}

template<typename PROXY>
auto
make_dynamic_entities(PROXY &proxy, LoadedEntities const& loaded_entites)
{
  std::vector<Transform*> entities;
  for (auto const& it : loaded_entites) {
    auto eid = proxy.create_entity();

    // assign through the reference
    auto &p = proxy.add_component(ct::transform, eid);
    *&p = it.transform;

    if (it.shader && it.color) {
    }

    entities.emplace_back(&p);
  }

  return entities;
}

template<typename PROXY>
auto
init(stlw::Logger &logger, PROXY &proxy, ImGuiIO &imgui, window::Dimensions const& dimensions,
    LoadedEntities const& entities_from_file)
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

  // TODO: HACK (FOR NOW, static entities are infront of the dynamic entities)
  auto dynamic_entities = make_dynamic_entities(proxy, entities_from_file);
  auto static_entities = make_entities(proxy);
  auto entities = stlw::combine_vectors(MOVE(static_entities), MOVE(dynamic_entities));

  auto &skybox_ent = *static_entities[SKYBOX_INDEX];
  auto &player_ent = *static_entities[AT_INDEX];
  player_ent.rotation = glm::angleAxis(glm::radians(180.0f), opengl::Y_UNIT_VECTOR);

  auto &light_ent = *static_entities[LIGHT_INDEX];
  light_ent.scale = glm::vec3{0.2f};

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -opengl::Z_UNIT_VECTOR;
  auto constexpr UP = opengl::Y_UNIT_VECTOR;
  Player player{player_ent, FORWARD, UP};
  {
    auto &startingpos = tmap_startingpos.second;
    auto const pos = glm::vec3{startingpos.x, startingpos.y, startingpos.z};
    //player.move_to(pos);
    light_ent.translation = pos;
  }
  Camera camera(proj, player_ent, FORWARD, UP);
  GameState gs{logger, imgui, dimensions, MOVE(rng), MOVE(tmap), MOVE(entities), MOVE(camera),
      MOVE(player), MOVE(Skybox{skybox_ent})};
  return MOVE(gs);
}

template<typename PROXY>
void
game_loop(GameState &state, PROXY &proxy, opengl::ShaderPrograms &sps, window::SDLWindow &window,
    DrawHandles &drawhandles, LoadedEntities const& entities_from_file)
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
  auto const& handles = drawhandles.handles;

  ///////////////////////////////////////////////////////////////////
  // drawing
  if (render.tilemap.redraw) {
    std::cerr << "Updating tilemap\n";
    update_visible_tiles(state.tilemap, player, render.tilemap.reveal);

    // We don't need to recompute the tilemap, we just did.
    state.render.tilemap.redraw = false;
  }

  render::clear_screen(render.background);

  // render entites from the file
  for (auto const& et : entities_from_file) {
    auto const shader_name = *et.shader;
    auto &shader_ref = sps.ref_sp(shader_name.c_str());

    if (et.shader && et.color) {
      auto handle = OF::copy_colorcube_gpu(logger, shader_ref, *et.color);
      render::draw(rargs, et.transform, shader_ref, handle);
      std::cerr << "dynamic entity color: '" << *et.color << "'\n";
    } else if (et.shader) {
      // TODO: TOTAL HACK (won't work, especially since they aren't added to level file yet.
      //auto &handle = handles.get(handle_name.c_str());
      //render::draw(rargs, et.transform, shader_ref, handle);
      std::abort();
    }
  }
  std::cerr << "\n\n";

  // light
  /*
  {
    auto light_handle = OF::copy_colorcube_gpu(logger, sps.ref_sp("light"), state.light.diffuse);
    render::draw(rargs, *ents[LIGHT_INDEX], sps.ref_sp("light"), light_handle);
  }

  // skybox
  state.skybox.transform.translation = ents[AT_INDEX]->translation;
  if (state.render.draw_skybox) {
    render::draw(rargs, state.skybox.transform, sps.ref_sp("skybox"), handles.get("SKYBOX"));
  }

  // random
  render::draw(rargs, *ents[TEXTURE_CUBE_INDEX], sps.ref_sp("3dcube_texture"), handles.get("TEXTURE_CUBE"));

  // house
  render::draw(rargs, *ents[HOUSE_INDEX], sps.ref_sp("3dtexture"), handles.get("HOUSE"));

  // tilemap
  render::draw_tilemap(rargs, *ents[TILEMAP_INDEX],
      {handles.get("HASHTAG"), sps.ref_sp("hashtag"), handles.get("PLUS"), sps.ref_sp("plus")},
      state.tilemap, state.render.tilemap.reveal);

  if (state.render.tilemap.show_grid_lines) {
    auto &sp = sps.ref_sp("3d_pos_color");
    auto const tilegrid = OF::create_tilegrid(logger, sp, state.tilemap,
        state.render.tilemap.show_yaxis_lines);
    render::draw_tilegrid(rargs, *ents[TILEMAP_INDEX], sp, tilegrid);
  }

  // player
  render::draw(rargs, *ents[AT_INDEX], sps.ref_sp("3d_pos_normal_color"), handles.get("AT"));

  // enemies
  render::draw(rargs, *ents[ORC_INDEX], sps.ref_sp("3d_pos_normal_color"), handles.get("ORC"));
  render::draw(rargs, *ents[TROLL_INDEX], sps.ref_sp("3d_pos_normal_color"), handles.get("TROLL"));

  // global coordinates
  if (state.render.show_global_axis) {
    render::draw(rargs, *ents[GLOBAL_AXIS_X_INDEX], sps.ref_sp("3d_pos_color"), handles.get("GLOBAL_AXIS_X"));
    render::draw(rargs, *ents[GLOBAL_AXIS_Y_INDEX], sps.ref_sp("3d_pos_color"), handles.get("GLOBAL_AXIS_X"));
    render::draw(rargs, *ents[GLOBAL_AXIS_Z_INDEX], sps.ref_sp("3d_pos_color"), handles.get("GLOBAL_AXIS_X"));
  }

  // local coordinates
  if (state.render.show_local_axis) {
    auto const& player_pos = ents[AT_INDEX]->translation;
    {
      auto const world_coords = OF::create_axis_arrows(logger,
          sps.ref_sp("3d_pos_color"),
          sps.ref_sp("3d_pos_color"),
          sps.ref_sp("3d_pos_color"),
          player_pos);
      render::draw(rargs, *ents[LOCAL_AXIS_X_INDEX], sps.ref_sp("3d_pos_color"), handles.get("LOCAL_AXIS_X"));
      render::draw(rargs, *ents[LOCAL_AXIS_Y_INDEX], sps.ref_sp("3d_pos_color"), handles.get("LOCAL_AXIS_Y"));
      render::draw(rargs, *ents[LOCAL_AXIS_Z_INDEX], sps.ref_sp("3d_pos_color"), handles.get("LOCAL_AXIS_Z"));
    }
  }

  // draw forward arrow (for player)
  if (state.render.show_target_vectors) {
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + (2.0f * player.forward_vector());

      auto const handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
          OF::ArrowCreateParams{LOC::LIGHT_BLUE, start, head});

      render::draw(rargs, *ents[LOCAL_FORWARD_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
    // draw forward arrow (for camera)
    {
      //glm::vec3 const start = glm::vec3{0.0f, 0.0f, 1.0f};
      //glm::vec3 const head = glm::vec3{0.0f, 0.0f, 2.0f};
      glm::vec3 const start = glm::vec3{0, 0, 0};
      glm::vec3 const head = state.ui_state.last_mouse_clicked_pos;

      auto handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
        OF::ArrowCreateParams{LOC::YELLOW, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS0_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
    // draw forward arrow (for camera)
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.backward_vector();

      auto const handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
        OF::ArrowCreateParams{LOC::PINK, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS2_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
    // draw arrow from origin -> camera
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.right_vector();

      auto const handle = OF::create_arrow(logger, sps.ref_sp("3d_pos_color"),
        OF::ArrowCreateParams{LOC::PURPLE, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS2_INDEX], sps.ref_sp("3d_pos_color"), handle);
    }
  }

  // terrain
  //render::draw(rargs, *ents[TERRAIN_INDEX], sps.ref_sp("terrain"), handles.terrain);

*/
  // UI code
  draw_ui(state, window);
}

} // ns boomhs
