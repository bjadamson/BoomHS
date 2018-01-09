#pragma once

#include <boost/optional.hpp>
#include <cpptoml/cpptoml.h>
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
using CppTableArray = std::shared_ptr<cpptoml::table_array>;
using CppTable = std::shared_ptr<cpptoml::table>;
namespace OF = opengl::factories;
namespace LOC = opengl::LIST_OF_COLORS;

namespace boomhs
{

auto
load_meshes(opengl::ObjLoader &loader, CppTableArray const& mesh_table)
{
  auto const load = [&loader](auto const& table) {
    auto const load_name = table->template get_as<std::string>("name");
    assert(load_name);

    auto const load_normals = table->template get_as<bool>("normals");
    assert(load_normals);

    auto const load_uvs = table->template get_as<bool>("uvs");
    assert(load_uvs);

    auto const obj = "assets/" + *load_name + ".obj";
    auto const mtl = "assets/" + *load_name + ".mtl";

    auto const normals = opengl::LoadNormals{*load_normals};
    auto const uvs = opengl::LoadUvs{*load_uvs};

    // cpptoml (not sure if TOML in general) won't let me have an array of floats,
    // so an array of doubles (buffer) is used to read from the file.
    auto const load_colors = table->template get_array_of<double>("color");
    if (load_colors) {
      opengl::Color color;
      color.r = (*load_colors)[0];
      color.g = (*load_colors)[1];
      color.b = (*load_colors)[2];
      color.a = (*load_colors)[3];
      loader.set_color(color);
    }
    auto mesh = loader.load_mesh(obj.c_str(), mtl.c_str(), normals, uvs);
    return std::make_pair(*load_name, MOVE(mesh));
  };
  ObjCache cache;
  for (auto const& table : *mesh_table) {
    auto pair = load(table);
    cache.add_obj(pair.first, MOVE(pair.second));
  }
  return cache;
}

class ParsedVertexAttributes
{
  std::vector<std::string> names_;
  std::vector<opengl::VertexAttribute> attributes_;
public:
  ParsedVertexAttributes() = default;

  void
  add(std::string const& name, opengl::VertexAttribute &&va)
  {
    names_.emplace_back(name);
    attributes_.emplace_back(MOVE(va));
  }

  auto
  take_va(std::string const& va_name)
  {
    auto const cmp = [&va_name](auto const& it) { return it == va_name; };
    auto const find = std::find_if(names_.cbegin(), names_.cend(), cmp);

    // TODO: for now assume we always find requested VA.
    assert(find != names_.cend());
    auto const index = std::distance(names_.cbegin(), find);

    return MOVE(attributes_[index]);
  }
};

stlw::result<opengl::ShaderPrograms, std::string>
load_shaders(stlw::Logger &logger, ParsedVertexAttributes &&pvas, CppTableArray const& shaders_table)
{
  auto const concat = [](auto const& prefix, std::size_t const index, auto const& suffix) {
    return prefix + std::to_string(index) + suffix;
  };
  auto const load = [&](auto const& table) -> stlw::result<std::pair<std::string, opengl::ShaderProgram>, std::string> {
    auto const load_name = table->template get_as<std::string>("name");
    assert(load_name);

    auto const load_vertex = table->template get_as<std::string>("vertex");
    assert(load_vertex);

    auto const load_fragment = table->template get_as<std::string>("fragment");
    assert(load_fragment);

    auto const load_va_name = table->template get_as<std::string>("va");
    assert(load_va_name);

    auto const va_name = *load_va_name;
    std::cerr << "na_name: '" << va_name << "'\n";
    auto va = pvas.take_va(va_name);

    char const* vertex = load_vertex->c_str();
    char const* fragment = load_vertex->c_str();
    auto const name = *load_name;
    DO_TRY(auto program, opengl::make_shader_program(vertex, fragment, MOVE(va)));
    return std::make_pair(name, MOVE(program));
  };

  opengl::ShaderPrograms sps;
  for (auto const& table : *shaders_table) {
    DO_TRY(auto pair, load(table));
    sps.add(pair.first, MOVE(pair.second));
  }
  std::abort();
}

#define TRY_OPTION_GENERAL_EVAL(VAR_DECL, V, expr)                                                 \
  auto V{expr};                                                                                    \
  if (!V) {                                                                                        \
    return cpptoml::option<opengl::AttributePointerInfo>{}; \
  }                                                                                                \
  VAR_DECL{MOVE(V)};

#define TRY_OPTION_CONCAT(VAR_DECL, TO_CONCAT, expr)                                               \
  TRY_OPTION_GENERAL_EVAL(VAR_DECL, _TRY_OPTION_TEMPORARY_##TO_CONCAT, expr)

#define TRY_OPTION_EXPAND_VAR(VAR_DECL, to_concat, expr)                                           \
  TRY_OPTION_CONCAT(VAR_DECL, to_concat, expr)

// TRY_OPTION
#define TRY_OPTION(VAR_DECL, expr)                                                                 \
  TRY_OPTION_EXPAND_VAR(VAR_DECL, __COUNTER__, expr)

auto
load_vas(CppTable const& config)
{
  auto const concat = [](auto const& prefix, std::size_t const index, auto const& suffix) {
    return prefix + std::to_string(index) + suffix;
  };

  auto const read_data = [&](auto const& table, std::size_t const index) {
    auto const dataname = "data" + std::to_string(index);
    TRY_OPTION(auto data_table, table->get_table(dataname.c_str()));
    assert(data_table);

    TRY_OPTION(auto type_o, data_table->template get_as<std::string>("type"));
    assert(type_o);

    // TODO: FOR NOW, only support floats. Easy to implement rest
    assert("float" == *type_o);
    auto const type = GL_FLOAT;
    TRY_OPTION(auto const num_o, data_table->template get_as<int>("num"));

    auto const uint_index = static_cast<GLuint>(index);
    auto api = opengl::AttributePointerInfo{uint_index, type, *num_o};
    return cpptoml::option<opengl::AttributePointerInfo>{MOVE(api)};
  };

  auto vas_table_array = config->get_table_array("vas");
  assert(vas_table_array);

  ParsedVertexAttributes pvas;
  for(auto const& va_table : *vas_table_array) {
    auto const name_o = va_table->get_as<std::string>("name");
    assert(name_o);
    auto const name = *name_o;

    auto const add_if = [&](auto &apis, std::size_t const index) {
      auto data = read_data(va_table, index);
      if (data) {
        apis.emplace_back(*data);
      }
    };

    std::vector<opengl::AttributePointerInfo> apis;
    add_if(apis, 0);
    add_if(apis, 1);
    add_if(apis, 2);
    pvas.add(name, make_vertex_attribute(apis));
  }
  return pvas;
}

struct Assets
{
  ObjCache obj_cache;
  opengl::ShaderPrograms shader_programs;
};

stlw::result<Assets, std::string>
load_assets(stlw::Logger &logger)
{
  CppTable table = cpptoml::parse_file("levels/area0");

  auto const shaders_table = table->get_table_array("shaders");
  assert(shaders_table);

  ParsedVertexAttributes pvas = load_vas(table);
  DO_TRY(auto shader_programs, load_shaders(logger, MOVE(pvas), shaders_table));

  auto const mesh_table = table->get_table_array("meshes");
  assert(mesh_table);

  auto loader = opengl::ObjLoader{LOC::WHITE};
  auto meshes = load_meshes(loader, mesh_table);

  return Assets{MOVE(meshes), MOVE(shader_programs)};
}

stlw::result<DrawHandles, std::string>
copy_assets_gpu(stlw::Logger &logger, ObjCache const& obj_cache, opengl::ShaderPrograms &sps)
{
  using namespace opengl;
  auto const copy = [&logger, &obj_cache](auto const mode, auto const& shader, char const* name) {
    auto const& obj = obj_cache.get_obj(name);
    auto handle = OF::copy_gpu(logger, mode, shader, obj);
    return handle;
  };

  // clang-format off
  GpuHandles handles;
  handles.set(HOUSE,  copy(GL_TRIANGLES,       sps.sp("house"),   "house_uv"));
  handles.set(TILEMAP, copy(GL_TRIANGLE_STRIP, sps.sp("hashtag"), "hashtag"));

  handles.set(HASHTAG, copy(GL_TRIANGLES, sps.sp("hashtag"), "hashtag"));
  handles.set(AT,      copy(GL_TRIANGLES, sps.sp("at"),      "at"));
  handles.set(PLUS,    copy(GL_TRIANGLES, sps.sp("plus"),    "plus"));
  handles.set(ORC,     copy(GL_TRIANGLES, sps.sp("O"),        "O"));
  handles.set(TROLL,   copy(GL_TRIANGLES, sps.sp("T"),        "T"));
  // clang-format on

  auto const copy_texturecube = [&logger](auto const& shader) {
    return OF::copy_texturecube_gpu(logger, shader);
  };
  handles.set(SKYBOX, copy_texturecube(sps.sp("skybox")));
  handles.set(TEXTURE_CUBE, copy_texturecube(sps.sp("texture_cube")));

  auto const copy_cubecolor = [&logger](auto const& shader, opengl::Color const color) {
    return OF::copy_colorcube_gpu(logger, shader, color);
  };
  handles.set(COLOR_CUBE, copy_cubecolor(sps.sp("color"), LOC::BLUE));
  handles.set(TERRAIN, copy_cubecolor(sps.sp("terrain"), LOC::SADDLE_BROWN));

  // world-axis
  auto world_arrows = OF::create_world_axis_arrows(logger, sps.sp("global_x_axis_arrow"),
      sps.sp("global_y_axis_arrow"), sps.sp("global_z_axis_arrow"));

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

  entities[COLOR_CUBE_INDEX]->translation = glm::vec3{-2.0f, -2.0f, -2.0f};
  entities[TEXTURE_CUBE_INDEX]->translation = glm::vec3{-4.0f, -4.0f, -2.0f};
  entities[SKYBOX_INDEX]->translation = glm::vec3{0.0f, 0.0f, 0.0f};
  entities[HOUSE_INDEX]->translation = glm::vec3{2.0f, 0.0f, -4.0f};
  entities[TERRAIN_INDEX]->translation = glm::vec3{0.0f, 5.0f, 0.0f};
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

  auto &skybox_ent = *entities[SKYBOX_INDEX];
  auto &player_ent = *entities[AT_INDEX];
  player_ent.rotation = glm::angleAxis(glm::radians(180.0f), opengl::Y_UNIT_VECTOR);

  auto &light_ent = *entities[LIGHT_INDEX];
  light_ent.scale = glm::vec3{0.2f};

  // camera-look at origin
  // cameraspace "up" is === "up" in worldspace.
  auto const FORWARD = -opengl::Z_UNIT_VECTOR;
  auto constexpr UP = opengl::Y_UNIT_VECTOR;
  Player player{player_ent, FORWARD, UP};
  {
    auto &startingpos = tmap_startingpos.second;
    auto const pos = glm::vec3{startingpos.x, startingpos.y, startingpos.z};
    player.move_to(pos);

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
void game_loop(GameState &state, PROXY &proxy, opengl::ShaderPrograms &sps, window::SDLWindow &window,
    DrawHandles &drawhandles)
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

  // light
  {
    auto light_handle = OF::copy_colorcube_gpu(logger, sps.sp("light0"), state.light.diffuse);
    render::draw(rargs, *ents[LIGHT_INDEX], sps.sp("light0"), light_handle);
  }

  // skybox
  state.skybox.transform.translation = ents[AT_INDEX]->translation;
  if (state.render.draw_skybox) {
    render::draw(rargs, state.skybox.transform, sps.sp("skybox"), handles.get("SKYBOX"));
  }

  // random
  render::draw(rargs, *ents[COLOR_CUBE_INDEX], sps.sp("color"), handles.get("COLOR_CUBE"));
  render::draw(rargs, *ents[TEXTURE_CUBE_INDEX], sps.sp("texture_cube"), handles.get("TEXTURE_CUBE"));

  // house
  render::draw(rargs, *ents[HOUSE_INDEX], sps.sp("house"), handles.get("HOUSE"));

  // tilemap
  render::draw_tilemap(rargs, *ents[TILEMAP_INDEX],
      {handles.get("HASHTAG"), sps.sp("hashtag"), handles.get("PLUS"), sps.sp("plus")},
      state.tilemap, state.render.tilemap.reveal);

  if (state.render.tilemap.show_grid_lines) {
    auto &sp = sps.sp("global_x_axis_arrow");
    auto const tilegrid = OF::create_tilegrid(logger, sp, state.tilemap,
        state.render.tilemap.show_yaxis_lines);
    render::draw_tilegrid(rargs, *ents[TILEMAP_INDEX], sp, tilegrid);
  }

  // player
  render::draw(rargs, *ents[AT_INDEX], sps.sp("at"), handles.get("AT"));

  // enemies
  render::draw(rargs, *ents[ORC_INDEX], sps.sp("O"), handles.get("ORC"));
  render::draw(rargs, *ents[TROLL_INDEX], sps.sp("T"), handles.get("TROLL"));

  // global coordinates
  if (state.render.show_global_axis) {
    render::draw(rargs, *ents[GLOBAL_AXIS_X_INDEX], sps.sp("global_x_axis_arrow"), handles.get("GLOBAL_AXIS_X"));
    render::draw(rargs, *ents[GLOBAL_AXIS_Y_INDEX], sps.sp("global_y_axis_arrow"), handles.get("GLOBAL_AXIS_X"));
    render::draw(rargs, *ents[GLOBAL_AXIS_Z_INDEX], sps.sp("global_z_axis_arrow"), handles.get("GLOBAL_AXIS_X"));
  }

  // local coordinates
  if (state.render.show_local_axis) {
    auto const& player_pos = ents[AT_INDEX]->translation;
    {
      auto const world_coords = OF::create_axis_arrows(logger,
          sps.sp("local_x_axis_arrow"),
          sps.sp("local_y_axis_arrow"),
          sps.sp("local_z_axis_arrow"),
          player_pos);
      render::draw(rargs, *ents[LOCAL_AXIS_X_INDEX], sps.sp("local_x_axis_arrow"), handles.get("LOCAL_AXIS_X"));
      render::draw(rargs, *ents[LOCAL_AXIS_Y_INDEX], sps.sp("local_y_axis_arrow"), handles.get("LOCAL_AXIS_Y"));
      render::draw(rargs, *ents[LOCAL_AXIS_Z_INDEX], sps.sp("local_z_axis_arrow"), handles.get("LOCAL_AXIS_Z"));
    }
  }

  // draw forward arrow (for player)
  if (state.render.show_target_vectors) {
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + (2.0f * player.forward_vector());

      auto const handle = OF::create_arrow(logger, sps.sp("local_forward_arrow"),
          OF::ArrowCreateParams{LOC::LIGHT_BLUE, start, head});

      render::draw(rargs, *ents[LOCAL_FORWARD_INDEX], sps.sp("local_forward_arrow"), handle);
    }
    // draw forward arrow (for camera)
    {
      //glm::vec3 const start = glm::vec3{0.0f, 0.0f, 1.0f};
      //glm::vec3 const head = glm::vec3{0.0f, 0.0f, 2.0f};
      glm::vec3 const start = glm::vec3{0, 0, 0};
      glm::vec3 const head = state.ui_state.last_mouse_clicked_pos;

      auto handle = OF::create_arrow(logger, sps.sp("camera_arrow0"),
        OF::ArrowCreateParams{LOC::YELLOW, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS0_INDEX], sps.sp("camera_arrow0"), handle);
    }
    // draw forward arrow (for camera)
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.backward_vector();

      auto const handle = OF::create_arrow(logger, sps.sp("camera_arrow1"),
        OF::ArrowCreateParams{LOC::PINK, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS2_INDEX], sps.sp("camera_arrow1"), handle);
    }
    // draw arrow from origin -> camera
    {
      glm::vec3 const start = player.world_position();
      glm::vec3 const head = start + player.right_vector();

      auto const handle = OF::create_arrow(logger, sps.sp("camera_arrow2"),
        OF::ArrowCreateParams{LOC::PURPLE, start, head});

      render::draw(rargs, *ents[CAMERA_LOCAL_AXIS2_INDEX], sps.sp("camera_arrow2"), handle);
    }
  }

  // terrain
  //render::draw(rargs, *ents[TERRAIN_INDEX], sps.sp("terrain"), handles.terrain);

  // UI code
  draw_ui(state, window);
}

} // ns boomhs
