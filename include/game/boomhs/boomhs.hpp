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

#include <game/boomhs/assets.hpp>
#include <game/boomhs/io_system.hpp>
#include <game/boomhs/randompos_system.hpp>
#include <game/boomhs/tilemap.hpp>

namespace game::boomhs
{

template <typename L>
struct RenderArgs {
  L &logger;
  opengl::camera const& camera;
  glm::mat4 const& projection;
};

template<typename L>
auto make_render_args(L &l, opengl::camera const& c, glm::mat4 const& projection)
{
  return RenderArgs<L>{l, c, projection};
}

template <typename L>
struct GameState {
  bool quit = false;
  L &logger;
  window::dimensions const dimensions;
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
  GameState(L &l, window::dimensions const &d, stlw::float_generator &&fg,
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

  RenderArgs<L> render_args() const
  {
    return make_render_args(this->logger, this->camera, this->projection);
  }
};

template<typename L, typename HW>
auto
make_state(L &logger, HW const& hw)
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
  return GameState<L>(logger, hw, MOVE(rng), MOVE(projection), MOVE(tmap));
}

class boomhs_game
{
  NO_COPY(boomhs_game);
public:
  MOVE_DEFAULT(boomhs_game);
  boomhs_game() = default;

  auto ecst_systems() const
  {
    return std::make_tuple(st::io_system, st::randompos_system);
  }

  template<typename PROXY, typename STATE>
  auto
  init(PROXY &proxy, STATE &state, opengl::opengl_pipelines &gfx)
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

    auto house_mesh = opengl::load_mesh("assets/house_uv.obj", opengl::LoadNormals{true}, opengl::LoadUvs{true});
    auto house_uv = OF::make_mesh(logger, gfx.d3.house,
        opengl::mesh_properties{GL_TRIANGLES, house_mesh});

    auto hashtag_mesh = opengl::load_mesh("assets/hashtag.obj", "assets/hashtag.mtl", opengl::LoadNormals{false}, opengl::LoadUvs{false});
    auto hashtag = OF::make_mesh(logger, gfx.d3.hashtag,
        opengl::mesh_properties{GL_TRIANGLES, hashtag_mesh});

    auto tilemap_handle = OF::copy_tilemap_gpu(logger, gfx.d3.hashtag,
        {GL_TRIANGLES, hashtag_mesh.vertices, hashtag_mesh.indices},
        state.tilemap);

    return make_assets(MOVE(house_mesh), MOVE(hashtag_mesh), MOVE(house_uv), MOVE(hashtag),
        MOVE(tilemap_handle));
  }

  template <typename L, typename ASSETS>
  void game_loop(GameState<L> &state, opengl::opengl_pipelines &gfx, ASSETS const& assets)
  {
    {
      auto const color = opengl::LIST_OF_COLORS::WHITE;
      glm::vec4 const vec4 = {color[0], color[1], color[2], 1.0};
      opengl::clear_screen(vec4);
    }

    auto const random_comp = [&]() { return state.rnum_generator.generate_0to1(); };
    auto const rc = [&]() {
      return glm::vec4{random_comp(), random_comp(), random_comp(), 1.0f};
    };

    auto &logger = state.logger;
    auto const height = 0.25f, width = 0.39f;

    namespace OF = opengl::factories;
    auto cube_skybox = OF::make_cube(logger, gfx.d3.skybox, {GL_TRIANGLE_STRIP, {10.0f, 10.0f, 10.0f}});

    auto args = state.render_args();

    opengl::draw(args, state.skybox_model, cube_skybox);

    auto cube_texture = OF::make_cube(logger, gfx.d3.texture_cube, {GL_TRIANGLE_STRIP,
        {0.15f, 0.15f, 0.15f}});

    auto cube_color = OF::make_cube(logger, gfx.d3.color, {GL_TRIANGLE_STRIP, {0.25f, 0.25f, 0.25f}},
        opengl::LIST_OF_COLORS::BLUE);

    auto cube_wf = OF::make_cube(logger, gfx.d3.wireframe, {GL_LINE_LOOP, {0.25f, 0.25f, 0.25f}});

    // first draw terrain
    {
      auto cube_terrain = OF::make_cube(logger, gfx.d3.color, {GL_TRIANGLE_STRIP, {2.0f, 0.4f, 2.0f}},
          opengl::LIST_OF_COLORS::SADDLE_BROWN);
      opengl::draw(args, state.terrain_model, cube_terrain);
    }

    // now draw entities
    opengl::draw(args, *state.MODELS[0], cube_color);
    opengl::draw(args, *state.MODELS[1], cube_texture);
    opengl::draw(args, *state.MODELS[2], cube_wf);

    // house
    opengl::draw(args, state.house_model, assets.buffers.house);

    // tilemap
    opengl::draw_tilemap(args, *state.MODELS[3], assets.buffers.tilemap_handle, state.tilemap);
  }
};

} // ns game::boomhs
