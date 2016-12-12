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

  friend struct context_factory;
};

class opengl_texture_context : public opengl_context
{
  texture_info texture_info_;

  // private
  opengl_texture_context(program &&p, vertex_attribute &&va, texture_info const t)
      : opengl_context(std::move(p), std::move(va))
      , texture_info_(t)
  {
  }

  NO_COPY(opengl_texture_context);
  NO_MOVE_ASSIGN(opengl_texture_context);

  friend struct context_factory;

public:
  // move-construction OK.
  opengl_texture_context(opengl_texture_context &&other)
      : opengl_context(std::move(other))
      , texture_info_(other.texture_info_)
  {
    other.texture_info_ = texture_info{0, 0};
  }

  inline auto texture() const { return this->texture_info_; }
};

class opengl_wireframe_context : public opengl_context
{
  std::array<float, 4> color_;

  // private
  opengl_wireframe_context(program &&p, vertex_attribute &&va, std::array<float, 4> const &c)
      : opengl_context(std::move(p), std::move(va))
      , color_(c)
  {
  }
  NO_COPY(opengl_wireframe_context);
  NO_MOVE_ASSIGN(opengl_wireframe_context);
  friend struct context_factory;

public:
  // move-construction OK
  opengl_wireframe_context(opengl_wireframe_context &&other)
      : opengl_context(std::move(other))
      , color_(other.color_)
  {
    other.color_ = other.color_;
  }
  inline auto color() const { return this->color_; }
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
  auto static make_opengl_context(L &logger, program &&p, vertex_attribute &&va)
  {
    return make<opengl_context>(logger, std::move(p), std::move(va));
  }

  template <typename L>
  auto static make_texture_opengl_context(L &logger, program &&p, char const *path,
                                          vertex_attribute &&va)
  {
    auto const tid = load_2d_texture(logger, path);
    return make<opengl_texture_context>(logger, std::move(p), std::move(va), tid);
  }

  template <typename L>
  auto static make_3dcube_texture_opengl_context(L &logger, program &&p, char const *path,
                                                 vertex_attribute &&va)
  {
    auto const tid = load_3d_texture(logger, path, path, path, path, path, path);
    return make<opengl_texture_context>(logger, std::move(p), std::move(va), tid);
  }

  template <typename L>
  auto static make_wireframe_opengl_context(L &logger, program &&p, vertex_attribute &&va,
                                            std::array<float, 3> const &c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return make<opengl_wireframe_context>(logger, std::move(p), std::move(va), color);
  }
};

} // ns engine::gfx::opengl
