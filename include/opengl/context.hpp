#pragma once
#include <opengl/colors.hpp>
#include <opengl/resources.hpp>
#include <opengl/global.hpp>
#include <opengl/image_data.hpp>
#include <opengl/shader_program.hpp>
#include <opengl/texture.hpp>

#include <backward/backward.hpp>

namespace opengl
{

class opengl_vao {
  GLuint vao_ = 0;

  static constexpr auto NUM_BUFFERS = 1;

  explicit opengl_vao()
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
  }

public:
  friend class context_factory;
  NO_COPY(opengl_vao);
  NO_MOVE_ASSIGN(opengl_vao);

  ~opengl_vao()
  {
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  // move-construction OK.
  opengl_vao(opengl_vao &&other)
      : vao_(other.vao_)
  {
    other.vao_ = 0;
  }

  inline auto gl_raw_value() const { return this->vao_; }
};


class color2d_context
{
  opengl_vao vao_;

  explicit color2d_context(opengl_vao &&b)
    : vao_(MOVE(b))
  {
  }
public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(color2d_context);

  static bool constexpr IS_2D = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  auto const& vao() const { return this->vao_; }
};

class color3d_context
{
  opengl_vao vao_;

  explicit color3d_context(opengl_vao &&b)
    : vao_(MOVE(b))
  {
  }

public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(color3d_context);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  auto const& vao() const { return this->vao_; }
};

class texture3d_context
{
  opengl_vao vao_;
  texture_info texture_info_;

  explicit texture3d_context(opengl_vao &&b, texture_info const t)
      : vao_(MOVE(b))
      , texture_info_(t)
  {
  }

public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(texture3d_context);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

class texture_3dcube_context
{
  opengl_vao vao_;
  texture_info texture_info_;
  explicit texture_3dcube_context(opengl_vao &&b, texture_info const t)
      : vao_(MOVE(b))
      , texture_info_(t)
  {
  }

public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(texture_3dcube_context);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

class skybox_context
{
  opengl_vao vao_;
  texture_info texture_info_;

  explicit skybox_context(opengl_vao &&b, texture_info const t)
      : vao_(MOVE(b))
      , texture_info_(t)
  {
  }

public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(skybox_context);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

class texture2d_context
{
  opengl_vao vao_;
  texture_info texture_info_;
  explicit texture2d_context(opengl_vao &&b, texture_info const t)
      : vao_(MOVE(b))
      , texture_info_(t)
  {
  }

public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(texture2d_context);

  static bool constexpr IS_2D = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

class wireframe2d_context
{
  opengl_vao vao_;
  std::array<float, 4> color_;

  explicit wireframe2d_context(opengl_vao &&b, std::array<float, 4> const &color)
      : vao_(MOVE(b))
      , color_(color)
  {
  }

public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(wireframe2d_context);

  static bool constexpr IS_2D = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = true;
  static bool constexpr HAS_TEXTURE = false;

  inline auto color() const { return this->color_; }
  auto const& vao() const { return this->vao_; }
};

class wireframe3d_context
{
  opengl_vao vao_;
  std::array<float, 4> color_;

  explicit wireframe3d_context(opengl_vao &&b, std::array<float, 4> const &color)
      : vao_(MOVE(b))
      , color_(color)
  {
  }

public:
  friend class context_factory;
  MOVE_CONSTRUCTIBLE_ONLY(wireframe3d_context);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = true;
  static bool constexpr HAS_TEXTURE = false;

  inline auto color() const { return this->color_; }
  auto const& vao() const { return this->vao_; }
};

class context_factory
{
  context_factory() = delete;

  template <typename T, typename L, typename... Args>
  auto static make(L &logger, Args &&... args)
  {
    global::log::clear_gl_errors();
    T ctx{opengl_vao{}, std::forward<Args>(args)...};
    LOG_ANY_GL_ERRORS(logger, "constructing context");
    return ctx;
  }

public:
  template <typename L>
  auto static make_color2d(L &logger)
  {
    return make<color2d_context>(logger);
  }

  template <typename L>
  auto static make_texture2d(L &logger, char const *path)
  {
    auto const image_data = load_image(logger, path);
    auto const tid = upload_2d_texture(logger, image_data);
    return make<texture2d_context>(logger, tid);
  }

  template <typename L>
  auto static make_color3d(L &logger)
  {
    return make<color3d_context>(logger);
  }

  template <typename L>
  auto static make_texture3d(L &logger, char const* path)
  {
    auto const image_data = load_image(logger, path);
    auto const tid = upload_2d_texture(logger, image_data);
    return make<texture3d_context>(logger, tid);
  }

  template <typename L, typename ...Paths>
  auto static make_texture3dcube(L &logger, Paths const&... paths)
  {
    auto const image_data = load_image(logger, paths...);
    auto const tid = upload_3dcube_texture(logger, image_data);
    return make<texture_3dcube_context>(logger, tid);
  }

  template <typename L, typename ...Paths>
  auto static make_skybox(L &logger, Paths const&... paths)
  {
    auto const image_data = load_image(logger, paths...);
    auto const tid = upload_3dcube_texture(logger, image_data);
    return make<skybox_context>(logger, tid);
  }

  template <typename L>
  auto static make_wireframe2d(L &logger, std::array<float, 3> const &c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make<wireframe2d_context>(logger, color);
  }

  template <typename L>
  auto static make_wireframe3d(L &logger, std::array<float, 3> const &c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make<wireframe3d_context>(logger, color);
  }
};

struct opengl_context2d
{
  color2d_context color;
  texture2d_context texture_wall;
  texture2d_context texture_container;
  wireframe2d_context wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_context2d);
};

struct opengl_context3d
{
  color3d_context color;
  texture_3dcube_context texture;
  texture3d_context house_texture;
  skybox_context skybox;
  wireframe3d_context wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_context3d);
};

struct opengl_contexts
{
  opengl_context2d d2;
  opengl_context3d d3;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_contexts);
  explicit opengl_contexts(opengl_context2d &&c2d, opengl_context3d &&c3d)
    : d2(MOVE(c2d))
    , d3(MOVE(c3d))
  {
  }
};

struct opengl_contexts_factory
{
  opengl_contexts_factory() = delete;

  template <typename L>
  static opengl_contexts make(L &logger)
  {
    using namespace opengl;
    auto constexpr RESOURCES = resources::make_resource_table();
    auto const get_r = [&](auto const i) { return RESOURCES[i]; };

    auto c0 = context_factory::make_color2d(logger);
    auto c1 = context_factory::make_texture2d(logger, get_r(IMAGES::WALL));
    auto c2 = context_factory::make_texture2d(logger, get_r(IMAGES::CONTAINER));
    auto const color = LIST_OF_COLORS::PINK;
    auto c3 = context_factory::make_wireframe2d(logger, color);

    auto c4 = context_factory::make_color3d(logger);
    auto c5 = context_factory::make_texture3dcube(logger,
        get_r(IMAGES::CUBE_FRONT),
        get_r(IMAGES::CUBE_RIGHT),
        get_r(IMAGES::CUBE_BACK),
        get_r(IMAGES::CUBE_LEFT),
        get_r(IMAGES::CUBE_TOP),
        get_r(IMAGES::CUBE_BOTTOM)
        );
    auto c6 = context_factory::make_texture3d(logger, get_r(IMAGES::HOUSE));
    auto c7 = context_factory::make_skybox(logger,
        get_r(IMAGES::SB_FRONT),
        get_r(IMAGES::SB_RIGHT),
        get_r(IMAGES::SB_BACK),
        get_r(IMAGES::SB_LEFT),
        get_r(IMAGES::SB_TOP),
        get_r(IMAGES::SB_BOTTOM)
        );
    auto const color2 = LIST_OF_COLORS::PURPLE;
    auto c8 = context_factory::make_wireframe3d(logger, color2);

    opengl_context2d d2{MOVE(c0), MOVE(c1), MOVE(c2), MOVE(c3)};
    opengl_context3d d3{MOVE(c4), MOVE(c5), MOVE(c6), MOVE(c7), MOVE(c8)};
    return opengl_contexts{MOVE(d2), MOVE(d3)};
  }
};

} // ns opengl
