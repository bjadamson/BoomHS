#pragma once
#include <gfx/colors.hpp>
#include <gfx/resources.hpp>
#include <gfx/opengl/global.hpp>
#include <gfx/opengl/pipeline.hpp>
#include <gfx/opengl/texture.hpp>
#include <iostream>

namespace gfx::opengl
{

class opengl_context
{
  GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  NO_COPY(opengl_context);
  NO_MOVE_ASSIGN(opengl_context);
protected:
  explicit opengl_context()
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
    glGenBuffers(NUM_BUFFERS, &this->ebo_);
  }
public:
  ~opengl_context()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->ebo_);
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  // move-construction OK.
  opengl_context(opengl_context &&other)
      : vao_(other.vao_)
      , vbo_(other.vbo_)
      , ebo_(other.ebo_)
  {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;
  }

  inline auto vao() const { return this->vao_; }
  inline auto vbo() const { return this->vbo_; }
  inline auto ebo() const { return this->ebo_; }

  // Derived context's can override this.
  static bool constexpr HAS_TEXTURE = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr IS_SKYBOX = false;
};

struct context2d : public opengl_context
{
  static bool constexpr IS_2D = true;
protected:
  explicit context2d() = default;
public:
  MOVE_CONSTRUCTIBLE_ONLY(context2d);
};

struct context3d : public opengl_context
{
  static bool constexpr IS_2D = false;
protected:
  explicit context3d() = default;
public:
  MOVE_CONSTRUCTIBLE_ONLY(context3d);
};

struct color2d_context : public context2d
{
  friend struct context_factory;
protected:
  explicit color2d_context() = default;
public:
  MOVE_CONSTRUCTIBLE_ONLY(color2d_context);
};

class color3d_context : public context3d
{
friend struct context_factory;
protected:
  explicit color3d_context() = default;
public:
  MOVE_CONSTRUCTIBLE_ONLY(color3d_context);
};

template<typename D>
class opengl_texture_context : public D
{
protected:
  texture_info texture_info_;

  // private
  explicit opengl_texture_context(texture_info const t)
      : texture_info_(t)
  {
  }

  NO_COPY(opengl_texture_context);
  NO_MOVE_ASSIGN(opengl_texture_context);
public:
  // move-construction OK.
  explicit opengl_texture_context(opengl_texture_context &&other)
      : texture_info_(other.texture_info_)
  {
    other.texture_info_ = texture_info{0, 0};
  }

  static bool constexpr HAS_TEXTURE = true;
  inline auto texture() const { return this->texture_info_; }
};

class texture3d_context : public opengl_texture_context<context3d>
{
protected:
  explicit texture3d_context(texture_info const t)
      : opengl_texture_context(t)
  {
  }

  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE_ONLY(texture3d_context);
};

class skybox_context : public texture3d_context
{
  // private
  explicit skybox_context(texture_info const t)
      : texture3d_context(t)
  {
  }

  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE_ONLY(skybox_context);
  static bool constexpr IS_SKYBOX = true;
};

class texture2d_context : public opengl_texture_context<context2d>
{
  // private
  explicit texture2d_context(texture_info const t)
      : opengl_texture_context(t)
  {
  }

  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE_ONLY(texture2d_context);
};

template<typename B>
class opengl_wireframe_context : public B
{
  std::array<float, 4> color_;

protected:
  explicit opengl_wireframe_context(std::array<float, 4> const &c)
      : color_(c)
  {
  }
public:
  MOVE_CONSTRUCTIBLE_ONLY(opengl_wireframe_context);

  static bool constexpr HAS_COLOR_UNIFORM = true;
  inline auto color() const { return this->color_; }
};

class wireframe2d_context : public opengl_wireframe_context<context2d>
{
  // private
  explicit wireframe2d_context(std::array<float, 4> const &c)
      : opengl_wireframe_context(c)
  {
  }

  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE_ONLY(wireframe2d_context);
};

class wireframe3d_context : public opengl_wireframe_context<context3d>
{
  // private
  explicit wireframe3d_context(std::array<float, 4> const &c)
      : opengl_wireframe_context(c)
  {
  }

  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE_ONLY(wireframe3d_context);
};

class context_factory
{
  context_factory() = delete;

  template <typename T, typename L, typename... Args>
  auto static make(L &logger, Args &&... args)
  {
    global::log::clear_gl_errors();
    T ctx{std::forward<Args>(args)...};
    LOG_ANY_GL_ERRORS(logger, "constructing context");
    return std::move(ctx);
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
    auto const tid = load_2d_texture(logger, path);
    return make<texture2d_context>(logger, tid);
  }

  template <typename L>
  auto static make_color3d(L &logger)
  {
    return make<color3d_context>(logger);
  }

  template <typename L, typename ...Paths>
  auto static make_texture3d(L &logger, Paths const&... paths)
  {
    auto const tid = load_3d_texture(logger, paths...);
    return make<texture3d_context>(logger, tid);
  }

  template <typename L, typename ...Paths>
  auto static make_skybox(L &logger, Paths const&... paths)
  {
    auto const tid = load_3d_texture(logger, paths...);
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

struct program2d
{
  pipeline<color2d_context> color;
  pipeline<texture2d_context> texture_wall;
  pipeline<texture2d_context> texture_container;
  pipeline<wireframe2d_context> wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(program2d);
};

struct program3d
{
  pipeline<color3d_context> color;
  pipeline<texture3d_context> texture;
  pipeline<skybox_context> skybox;
  pipeline<wireframe3d_context> wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(program3d);
};

struct opengl_contexts
{
  program2d d2;
  program3d d3;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_contexts);

  explicit opengl_contexts(program2d &&p2d, program3d &&p3d)
    : d2(MOVE(p2d))
    , d3(MOVE(p3d))
  {
  }
};

struct opengl_contexts_factory
{
  opengl_contexts_factory() = delete;

  template <typename L>
  static stlw::result<opengl_contexts, std::string> make(L &logger)
  {
    auto constexpr RESOURCES = resources::make_resource_table();
    auto const get_r = [&](auto const i) { return RESOURCES[i]; };

    pipeline_factory pf;
    va_factory vf;
    auto c0 = context_factory::make_color2d(logger);
    auto va0 = vf.make_vertex_color(logger);
    DO_TRY(auto p0, pf.make("2dcolor.vert", "2dcolor.frag", MOVE(c0), MOVE(va0)));

    auto c1 = context_factory::make_texture2d(logger, get_r(IMAGES::WALL));
    auto va1 = vf.make_vertex_uv2d(logger);
    DO_TRY(auto p1, pf.make("2dtexture.vert", "2dtexture.frag", MOVE(c1), MOVE(va1)));

    auto c2 = context_factory::make_texture2d(logger, get_r(IMAGES::CONTAINER));
    auto va2 = vf.make_vertex_uv2d(logger);
    DO_TRY(auto p2, pf.make("2dtexture.vert", "2dtexture.frag", MOVE(c2), MOVE(va2)));

    auto const color = LIST_OF_COLORS::PINK;
    auto c3 = context_factory::make_wireframe2d(logger, color);
    auto va3 = vf.make_vertex_only(logger);
    DO_TRY(auto p3, pf.make("wire.vert", "wire.frag", MOVE(c3), MOVE(va3)));

    auto c4 = context_factory::make_color3d(logger);
    auto va4 = vf.make_vertex_color(logger);
    DO_TRY(auto p4, pf.make("3dcolor.vert", "3dcolor.frag", MOVE(c4), MOVE(va4)));

    auto c5 = context_factory::make_texture3d(logger,
        get_r(IMAGES::CUBE_FRONT),
        get_r(IMAGES::CUBE_RIGHT),
        get_r(IMAGES::CUBE_BACK),
        get_r(IMAGES::CUBE_LEFT),
        get_r(IMAGES::CUBE_TOP),
        get_r(IMAGES::CUBE_BOTTOM)
        );
    auto va5 = vf.make_vertex_only(logger);
    DO_TRY(auto p5, pf.make("3dtexture.vert", "3dtexture.frag", MOVE(c5), MOVE(va5)));

    auto c6 = context_factory::make_skybox(logger,
        get_r(IMAGES::SB_FRONT),
        get_r(IMAGES::SB_RIGHT),
        get_r(IMAGES::SB_BACK),
        get_r(IMAGES::SB_LEFT),
        get_r(IMAGES::SB_TOP),
        get_r(IMAGES::SB_BOTTOM)
        );
    auto va6 = vf.make_vertex_only(logger);
    DO_TRY(auto p6, pf.make("3dtexture.vert", "3dtexture.frag", MOVE(c6), MOVE(va6)));

    auto const color2 = LIST_OF_COLORS::PURPLE;
    auto c7 = context_factory::make_wireframe3d(logger, color2);
    auto va7 = vf.make_vertex_only(logger);
    DO_TRY(auto p7, pf.make("3dwire.vert", "wire.frag", MOVE(c7), MOVE(va7)));

    program2d d2{MOVE(p0), MOVE(p1), MOVE(p2), MOVE(p3)};
    program3d d3{MOVE(p4), MOVE(p5), MOVE(p6), MOVE(p7)};
    return opengl_contexts{MOVE(d2), MOVE(d3)};
  }
};

} // ns gfx::opengl
