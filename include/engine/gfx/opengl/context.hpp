#pragma once
#include <SOIL.h>
#include <engine/gfx/colors.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/vertex_attribute.hpp>
#include <iostream>

namespace engine::gfx::opengl
{

namespace impl
{

static auto const load_texture = [](char const *path) {
  GLuint texture;
  glGenTextures(1, &texture);

  global::texture_bind(texture);
  ON_SCOPE_EXIT([]() { global::texture_unbind(); });

  // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  int w = 0, h = 0;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, SOIL_LOAD_RGB);
  if (nullptr == pimage) {
    std::cerr << "image '" << path << "' didn't load.";
    std::abort();
  }
  ON_SCOPE_EXIT([&]() { SOIL_free_image_data(pimage); });

  // actually send the texture to the GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pimage);
  glGenerateMipmap(GL_TEXTURE_2D);

  return texture;
};

} // ns impl

class opengl_context
{
  GLuint vao_ = 0, vbo_ = 0;
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

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
    ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });
  }

public:
  ~opengl_context()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  // move-construction OK.
  opengl_context(opengl_context &&other)
      : vao_(other.vao_)
      , vbo_(other.vbo_)
      , program_(std::move(other.program_))
      , va_(std::move(other.va_))
  {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.program_ = program::make_invalid();
  }

  inline auto vao() const { return this->vao_; }
  inline auto vbo() const { return this->vbo_; }
  inline auto &program_ref() { return this->program_; }
  inline auto const& va() { return this->va_; }

  friend struct context_factory;
};

class opengl_texture_context : public opengl_context
{
  GLuint texture_;

  // private
  opengl_texture_context(program &&p, vertex_attribute &&va, GLuint const t)
    : opengl_context(std::move(p), std::move(va))
    , texture_(t)
  {
  }

  NO_COPY(opengl_texture_context);
  NO_MOVE_ASSIGN(opengl_texture_context);

  friend struct context_factory;
public:
  // move-construction OK.
  opengl_texture_context(opengl_texture_context &&other)
    : opengl_context(std::move(other))
    , texture_(other.texture_)
  {
    other.texture_ = 0;
  }

  inline auto texture() const { return this->texture_; }
};

class opengl_wireframe_context : public opengl_context
{
  std::array<float, 4> color_;

  // private
  opengl_wireframe_context(program &&p, vertex_attribute &&va, std::array<float, 4> const& c)
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

struct context_factory {
  context_factory() = delete;

  auto static make_opengl_context(program &&p, vertex_attribute &&va)
  {
    return opengl_context{std::move(p), std::move(va)};
  }

  auto static make_texture_opengl_context(program &&p, char const *path, vertex_attribute &&va)
  {
    auto const tid = impl::load_texture(path);
    return opengl_texture_context{std::move(p), std::move(va), tid};
  }

  auto static make_wireframe_opengl_context(program &&p, vertex_attribute &&va,
      std::array<float, 3> const& c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return opengl_wireframe_context{std::move(p), std::move(va), color};
  }
};

} // ns engine::gfx::opengl
