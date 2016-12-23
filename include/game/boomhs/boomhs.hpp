#pragma once
#include <string>
#include <vector>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <window/window.hpp>
#include <gfx/camera.hpp>
#include <gfx/factory.hpp>
#include <gfx/skybox.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/burrito.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/boomhs/ecst.hpp>
#include <game/boomhs/io_system.hpp>
#include <game/boomhs/randompos_system.hpp>
#include <game/data_types.hpp>

namespace game::boomhs
{

template <typename L, typename R>
struct game_state {
  bool quit = false;
  L &logger;
  R &renderer;
  window::dimensions const dimensions;
  stlw::float_generator rnum_generator;

  glm::mat4 projection;
  gfx::camera camera;

  NO_COPY(game_state);
  NO_MOVE_ASSIGN(game_state);
public:
  MOVE_CONSTRUCTIBLE(game_state);
  game_state(L &l, R &r, window::dimensions const &d, stlw::float_generator &&fg,
      glm::mat4 &&pm, gfx::camera &&c)
    : logger(l)
    , renderer(r)
    , dimensions(d)
    , rnum_generator(std::move(fg))
    , projection(std::move(pm))
    , camera(std::move(c))
  {
  }
};

// TODO: bye globals
std::vector<::gfx::model*> MODELS;
::gfx::model skybox_model;

template<typename L, typename R, typename HW>
auto
make_state(L &logger, R &renderer, HW const& hw)
{
  auto const fheight = static_cast<GLfloat>(hw.h);
  auto const fwidth = static_cast<GLfloat>(hw.w);

  logger.error(fmt::sprintf("fheight '%f', fwidth '%f'", fheight, fwidth));
  auto projection = glm::perspective(45.0f, (fwidth / fheight), 0.1f, 10000.0f);
  gfx::skybox sb{skybox_model};
  gfx::camera c{std::move(sb),
    glm::vec3(0.0f, 0.0f, 2.0f),  // camera position
    glm::vec3(0.0f, 0.0f, -1.0f), // look at origin
    glm::vec3(0.0f, 1.0f, 0.0f)}; // "up" vector
  stlw::float_generator rng;
  return game_state<L, R>(logger, renderer, hw, std::move(rng), std::move(projection), std::move(c));
}

template <typename G, typename S>
void
ecst_main(G &game, S &state)
{
  auto &logger = state.logger;
  logger.trace("creating ecst context ...");

  // Create an ECST context.
  auto ctx = ecst_setup::make_context();
  logger.trace("stepping ecst once");

  // Initialize context with some entities.
  ctx->step([&](auto &proxy)
      {
        auto const make_entity = [&proxy](auto const i, auto const& t) {
          auto eid = proxy.create_entity();
          auto *p = &proxy.add_component(ct::model, eid);
          p->translation = t;
          return p;
        };
        for (auto i{0}; i < 100; ++i) {
          auto const x = state.rnum_generator.generate_position();
          auto const y = state.rnum_generator.generate_position();
          auto const z = 0.0f;
          logger.info(fmt::sprintf("BEN, x '%f', y '%f'", x, y));
          MODELS.emplace_back(make_entity(i, glm::vec3{x, y, z}));
        }
  });

  namespace sea = ecst::system_execution_adapter;
  auto io_tags = sea::t(st::io_system);
  auto randompos_tags = sea::t(st::randompos_system);

  auto const init_system = [&logger](auto &system, auto &data) { system.init(logger); };
  auto const process_system = [&state, &logger](auto &system, auto &data) {
    system.process(data, state);
  };

  auto const io_init_system = io_tags.for_subtasks(init_system);
  auto const io_process_system = io_tags.for_subtasks(process_system);

  auto const randompos_init_system = randompos_tags.for_subtasks(init_system);
  auto const randompos_process_system = randompos_tags.for_subtasks(process_system);

  auto const game_loop_body = [&](auto &proxy) {
    logger.trace("executing systems.");
    proxy.execute_systems()(io_process_system, randompos_process_system);

    logger.trace("rendering.");
    game.game_loop(state);
    logger.trace("game loop stepping.");
  };
  auto const game_loop = [&state, &game_loop_body](auto &proxy) {
    while (!state.quit) {
      game_loop_body(proxy);
    }
  };
  ctx->step([&](auto &proxy) {
    logger.trace("game started, initializing systems.");
    proxy.execute_systems()(io_init_system, randompos_init_system);
    logger.trace("systems initialized, entering main loop.");

    game_loop(proxy);
    logger.trace("game loop finished.");
  });
}

template<typename D, typename C>
struct opengl_shape
{
  D data;
  C &context;

  opengl_shape(D &&d, C &c) : data(std::move(d)), context(c) {}
  MOVE_CONSTRUCTIBLE_ONLY(opengl_shape);
};

template<typename D, typename C>
auto ms(D &&d, C &c)
{
  return opengl_shape<D, C>(std::move(d), c);
}

class boomhs_game
{
  NO_COPY(boomhs_game);

public:
  boomhs_game() = default;
  MOVE_DEFAULT(boomhs_game);

  template <typename State>
  void game_loop(State &state)
  {
    ::gfx::render_args<decltype(state.logger)> const args{state.logger, state.camera,
                                                                  state.projection};

    using COLOR_ARRAY = std::array<float, 4>;
    auto constexpr multicolor_triangle =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(gfx::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::BLUE, 1.0f));

    auto constexpr multicolor_rect =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(gfx::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::BLUE, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::YELLOW, 1.0f));

    using sf = gfx::shape_factory;
    namespace gfx = gfx;

    // first, pick random shape
    auto const random_mode = [&]() {
      return static_cast<gfx::draw_mode>(gfx::draw_mode::TRIANGLES);
    };

    auto const random_comp = [&]() { return state.rnum_generator.generate_0to1(); };
    auto const random_color = [&]() {
      return glm::vec4{random_comp(), random_comp(), random_comp(), 1.0f};
    };

    auto const height = 0.25f, width = 0.39f;
    auto cube_skybox = sf::make_textured_cube(gfx::draw_mode::TRIANGLE_STRIP, skybox_model, {15.0f, 15.0f, 15.0f});


    auto &rd = state.renderer;
    auto &r = rd.gfx;
    auto &d2 = r.gfx_engine.d2;
    auto &d3 = r.gfx_engine.d3;
    rd.begin();
    auto x = ms(std::move(cube_skybox), d3.skybox);
    //r.draw_special(args, std::move(x));
    r.draw(args, d3.skybox, std::move(cube_skybox));
    {
      std::array<gfx::triangle<gfx::vertex_color_attributes>, 2> const arr = {
        gfx::triangle_factory::make(random_mode(), *MODELS[0], random_color()),
        gfx::triangle_factory::make(random_mode(), *MODELS[1], random_color())
      };
      r.draw(args, d2.color, arr);
    }
    {
      std::array<gfx::triangle<gfx::vertex_color_attributes>, 2> arr = {
        gfx::triangle_factory::make(random_mode(), *MODELS[2], random_color()),
        gfx::triangle_factory::make(random_mode(), *MODELS[3], random_color())
      };
      r.draw(args, d2.color, std::move(arr));
    }
    {
      r.draw(args, d2.color, std::make_tuple(
            gfx::triangle_factory::make(random_mode(), *MODELS[4], random_color())
            ));
    }
    {
      r.draw(args, d2.color,
            gfx::triangle_factory::make(random_mode(), *MODELS[5], random_color()),
            gfx::triangle_factory::make(random_mode(), *MODELS[6], random_color())
            );
    }
    {
      std::vector<gfx::triangle<gfx::vertex_color_attributes>> vec;
      vec.emplace_back(gfx::triangle_factory::make(random_mode(), *MODELS[7], random_color()));
      vec.emplace_back(gfx::triangle_factory::make(random_mode(), *MODELS[8], random_color()));

      r.draw(args, d2.color, std::move(vec));
    }

    auto triangle_color = sf::make_triangle(gfx::draw_mode::TRIANGLES, *MODELS[9], ::gfx::LIST_OF_COLORS::PINK);
    auto triangle_list_colors = sf::make_triangle(gfx::draw_mode::TRIANGLES, *MODELS[10], multicolor_triangle);
    auto triangle_texture = sf::make_triangle(gfx::draw_mode::TRIANGLES, *MODELS[11], true);
    auto triangle_wireframe = sf::make_triangle(gfx::draw_mode::LINE_LOOP, *MODELS[12], true, false);

    auto cube_texture = sf::make_textured_cube(gfx::draw_mode::TRIANGLE_STRIP, *MODELS[13], {0.15f, 0.15f, 0.15f});
    auto cube_color = sf::make_spotted_cube(gfx::draw_mode::TRIANGLE_STRIP, *MODELS[14], ::gfx::LIST_OF_COLORS::BLUE,
        {0.25f, 0.25f, 0.25f});
    auto cube_wf = sf::make_wireframe_cube(gfx::draw_mode::LINE_LOOP, *MODELS[15], {0.25f, 0.25f, 0.25f});

    auto rectangle_color = sf::make_rectangle({gfx::draw_mode::TRIANGLE_STRIP, *MODELS[16]},
        gfx::color_t{}, ::gfx::LIST_OF_COLORS::YELLOW);
    auto rectangle_list_colors = sf::make_rectangle({gfx::draw_mode::TRIANGLE_STRIP, *MODELS[17], height, width},
        gfx::color_t{}, multicolor_rect);
    auto rectangle_texture = sf::make_rectangle({gfx::draw_mode::TRIANGLE_STRIP, *MODELS[18], height, width}, gfx::uv_t{});
    auto rectangle_wireframe = sf::make_rectangle({gfx::draw_mode::LINE_LOOP, *MODELS[19], height, width}, gfx::wireframe_t{});

    auto polygon_color = gfx::polygon_factory::make({gfx::draw_mode::TRIANGLE_FAN, *MODELS[20], 5}, gfx::color_t{},
        ::gfx::LIST_OF_COLORS::DARK_ORANGE);

    auto polygon_texture = gfx::polygon_factory::make({gfx::draw_mode::TRIANGLE_FAN, *MODELS[21], 7}, gfx::uv_t{});
    auto polygon_wireframe = gfx::polygon_factory::make({gfx::draw_mode::LINE_LOOP, *MODELS[22], 7}, gfx::wireframe_t{});
    //auto polygon_list_of_color = gfx::polygon_factory::make(gfx::draw_mode::TRIANGLE_FAN, *zp1, 5, multicolor_triangle);

    r.draw(args, d2.color, triangle_color, triangle_list_colors, std::move(polygon_color),
                                           rectangle_color, rectangle_list_colors
                                           // polygon_list_of_color,
                                           );

    r.draw(args, d2.texture_wall, triangle_texture, rectangle_texture);//, polygon_texture);
    r.draw(args, d2.texture_container, std::move(polygon_texture));
    r.draw(args, d2.wireframe, std::move(polygon_wireframe), triangle_wireframe, rectangle_wireframe);

    r.draw(args, d3.color, cube_color);
    r.draw(args, d3.texture, cube_texture);
    r.draw(args, d3.wireframe, cube_wf);
    rd.end();
  }
};

} // ns game::boomhs
