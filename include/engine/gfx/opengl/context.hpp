#pragma once
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <engine/gfx/opengl/vertex_attrib.hpp>

namespace engine::gfx::opengl
{

namespace impl {

auto const load_texture = [](char const *path) {
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
    std::cerr << "image didn't load.";
    std::abort();
  }
  ON_SCOPE_EXIT([&]() { SOIL_free_image_data(pimage); });

  // actually send the texture to the GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pimage);
  glGenerateMipmap(GL_TEXTURE_2D);

  return texture;
};

} // ns impl

class render_context {
  GLuint vao_ = 0, vbo_ = 0;
  GLuint texture_ = 0;
  program program_;

  static auto constexpr NUM_BUFFERS = 1;

  NO_COPY(render_context);
  NO_MOVE_ASSIGN(render_context);
public:
  render_context(program &&p) : program_(std::move(p))
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
    glGenBuffers(NUM_BUFFERS, &this->vbo_);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_);
    ON_SCOPE_EXIT([]() { glBindBuffer(GL_ARRAY_BUFFER, 0); });

    this->texture_ = impl::load_texture("assets/container.jpg");
  }

  ~render_context()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  // move-construction OK.
  render_context(render_context &&other)
      : vao_(other.vao_)
      , vbo_(other.vbo_)
      , texture_(other.texture_)
      , program_(std::move(other.program_))
  {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.texture_ = 0;
    other.program_ = program::make_invalid();
  }

  inline auto vao() const { return this->vao_; }
  inline auto vbo() const { return this->vbo_; }
  inline auto texture() const { return this->texture_; }
  inline auto& program_ref() { return this->program_; }
};

} // ns engine::gfx::opengl
