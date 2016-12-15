#pragma once
#include <engine/gfx/colors.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/texture.hpp>
#include <engine/gfx/opengl/vertex_attribute.hpp>
#include <iostream>

namespace engine::gfx::opengl
{

class opengl_context
{
  GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
  program program_;
  vertex_attribute va_;

  static auto constexpr NUM_BUFFERS = 1;

  NO_COPY(opengl_context);
  NO_MOVE_ASSIGN(opengl_context);

protected:
  explicit opengl_context(program &&p, vertex_attribute &&va)
      : program_(std::move(p))
      , va_(std::move(va))
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
      , program_(std::move(other.program_))
      , va_(std::move(other.va_))
  {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;
    other.program_ = program::make_invalid();
  }

  inline auto vao() const { return this->vao_; }
  inline auto vbo() const { return this->vbo_; }
  inline auto ebo() const { return this->ebo_; }
  inline auto &program_ref() { return this->program_; }
  inline auto const &va() { return this->va_; }

  // Derived context's can override this.
  static bool constexpr HAS_TEXTURE = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
};

struct context2d : public opengl_context
{
  static bool constexpr IS_2D = true;
protected:
  explicit context2d(program &&p, vertex_attribute &&va)
    : opengl_context(std::move(p), std::move(va))
  {
  }
  NO_COPY(context2d);
  NO_MOVE_ASSIGN(context2d);
public:
  MOVE_CONSTRUCTIBLE(context2d);
};

struct context3d : public opengl_context
{
  static bool constexpr IS_2D = false;
protected:
  explicit context3d(program &&p, vertex_attribute &&va)
    : opengl_context(std::move(p), std::move(va))
  {
  }
  NO_COPY(context3d);
  NO_MOVE_ASSIGN(context3d);
public:
  MOVE_CONSTRUCTIBLE(context3d);
};

struct color2d_context : public context2d
{
  friend struct context_factory;
protected:
  explicit color2d_context(program &&p, vertex_attribute &&va)
    : context2d(std::move(p), std::move(va))
  {
  }
  NO_COPY(color2d_context);
  NO_MOVE_ASSIGN(color2d_context);
public:
  MOVE_CONSTRUCTIBLE(color2d_context);
};

class color3d_context : public context3d
{
friend struct context_factory;
protected:
  explicit color3d_context(program &&p, vertex_attribute &&va)
    : context3d(std::move(p), std::move(va))
  {
  }
  NO_COPY(color3d_context);
  NO_MOVE_ASSIGN(color3d_context);
public:
  MOVE_CONSTRUCTIBLE(color3d_context);
};

template<typename D>
class opengl_texture_context : public D
{
protected:
  texture_info texture_info_;

  // private
  explicit opengl_texture_context(program &&p, vertex_attribute &&va, texture_info const t)
      : D(std::move(p), std::move(va))
      , texture_info_(t)
  {
  }

  NO_COPY(opengl_texture_context);
  NO_MOVE_ASSIGN(opengl_texture_context);

public:
  // move-construction OK.
  explicit opengl_texture_context(opengl_texture_context &&other)
      : D(std::move(other))
      , texture_info_(other.texture_info_)
  {
    other.texture_info_ = texture_info{0, 0};
  }

  static bool constexpr HAS_TEXTURE = true;
  inline auto texture() const { return this->texture_info_; }
};

class texture3d_context : public opengl_texture_context<context3d>
{
  // private
  explicit texture3d_context(program &&p, vertex_attribute &&va, texture_info const t)
      : opengl_texture_context(std::move(p), std::move(va), t)
  {
  }

  NO_COPY(texture3d_context);
  NO_MOVE_ASSIGN(texture3d_context);
  friend struct context_factory;

public:
  MOVE_CONSTRUCTIBLE(texture3d_context);
};

class texture2d_context : public opengl_texture_context<context2d>
{
  // private
  explicit texture2d_context(program &&p, vertex_attribute &&va, texture_info const t)
      : opengl_texture_context(std::move(p), std::move(va), t)
  {
  }

  NO_COPY(texture2d_context);
  NO_MOVE_ASSIGN(texture2d_context);
  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE(texture2d_context);
};

template<typename B>
class opengl_wireframe_context : public B
{
  std::array<float, 4> color_;

protected:
  explicit opengl_wireframe_context(program &&p, vertex_attribute &&va, std::array<float, 4> const &c)
      : B(std::move(p), std::move(va))
      , color_(c)
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
  explicit wireframe2d_context(program &&p, vertex_attribute &&va, std::array<float, 4> const &c)
      : opengl_wireframe_context(std::move(p), std::move(va), c)
  {
  }

  NO_COPY(wireframe2d_context);
  NO_MOVE_ASSIGN(wireframe2d_context);
  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE(wireframe2d_context);
};

class wireframe3d_context : public opengl_wireframe_context<context3d>
{
  // private
  explicit wireframe3d_context(program &&p, vertex_attribute &&va, std::array<float, 4> const &c)
      : opengl_wireframe_context(std::move(p), std::move(va), c)
  {
  }

  NO_COPY(wireframe3d_context);
  NO_MOVE_ASSIGN(wireframe3d_context);
  friend struct context_factory;
public:
  MOVE_CONSTRUCTIBLE(wireframe3d_context);
};

class context_factory
{
  context_factory() = delete;

  template <typename T, typename L, typename... Args>
  auto static make(L &logger, Args &&... args)
  {
    global::log::clear_gl_errors();
    T ctx{std::forward<Args>(args)...};
    LOG_GL_ERRORS(logger, "constructing context");
    return std::move(ctx);
  }

public:
  template <typename L>
  auto static make_color2d(L &logger, program &&p, vertex_attribute &&va)
  {
    return make<color2d_context>(logger, std::move(p), std::move(va));
  }

  template <typename L>
  auto static make_texture2d(L &logger, program &&p, char const *path,
                                          vertex_attribute &&va)
  {
    auto const tid = load_2d_texture(logger, path);
    return make<texture2d_context>(logger, std::move(p), std::move(va), tid);
  }

  template <typename L>
  auto static make_color3d(L &logger, program &&p, vertex_attribute &&va)
  {
    return make<color3d_context>(logger, std::move(p), std::move(va));
  }

  template <typename L, typename ...Paths>
  auto static make_texture3d(L &logger, program &&p, vertex_attribute &&va,
      Paths const&... paths)
  {
    auto const tid = load_3d_texture(logger, paths...);
    return make<texture3d_context>(logger, std::move(p), std::move(va), tid);
  }

  template <typename L>
  auto static make_wireframe2d(L &logger, program &&p, vertex_attribute &&va,
                                            std::array<float, 3> const &c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make<wireframe2d_context>(logger, std::move(p), std::move(va), color);
  }

  template <typename L>
  auto static make_wireframe3d(L &logger, program &&p, vertex_attribute &&va,
                                            std::array<float, 3> const &c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make<wireframe3d_context>(logger, std::move(p), std::move(va), color);
  }
};

} // ns engine::gfx::opengl
