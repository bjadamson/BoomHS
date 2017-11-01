#pragma once
#include <array>
#include <opengl/types.hpp>
#include <opengl/colors.hpp>
#include <opengl/obj.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

class gpu_buffers
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit gpu_buffers()
  {
    glGenBuffers(NUM_BUFFERS, &this->vbo_);
    glGenBuffers(NUM_BUFFERS, &this->ebo_);
  }

  NO_COPY(gpu_buffers);
  NO_MOVE_ASSIGN(gpu_buffers);
public:
  friend class shape;
  ~gpu_buffers()
  {
    glDeleteBuffers(NUM_BUFFERS, &this->ebo_);
    glDeleteBuffers(NUM_BUFFERS, &this->vbo_);
  }

  // move-construction OK.
  gpu_buffers(gpu_buffers &&other)
      : vbo_(other.vbo_)
      , ebo_(other.ebo_)
  {
    other.vbo_ = 0;
    other.ebo_ = 0;
  }

  inline auto vbo() const { return this->vbo_; }
  inline auto ebo() const { return this->ebo_; }
};

class shape {
  GLenum draw_mode_;
  gpu_buffers gpu_buffers_;
  bool in_gpu_memory_ = false;

protected:
  explicit shape(GLenum const dm)
      : draw_mode_(dm)
  {
  }

public:

  auto constexpr draw_mode() const { return this->draw_mode_; }

  inline auto vbo() const { return this->gpu_buffers_.vbo(); }
  inline auto ebo() const { return this->gpu_buffers_.ebo(); }

  bool is_in_gpu_memory() const { return this->in_gpu_memory_; }
  void set_is_in_gpu_memory(bool const v) { this->in_gpu_memory_ = v; }
};

struct vertex_attributes_only {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_color_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX + color_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_uv_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX + uv_t::NUM_FLOATS_PER_VERTEX;
};

struct vertex_normal_uv_attributes {
  static constexpr auto NUM_FLOATS_PER_VERTEX = vertex_t::NUM_FLOATS_PER_VERTEX
    + normal_t::NUM_FLOATS_PER_VERTEX
    + uv_t::NUM_FLOATS_PER_VERTEX;
};

template <typename V>
class mesh : public shape {
  obj object_data_;

public:
  friend class mesh_factory;
  MOVE_CONSTRUCTIBLE_ONLY(mesh);

  explicit constexpr mesh(GLenum const dm, obj &&object)
      : shape(dm)
      , object_data_(MOVE(object))
  {
  }

  auto num_vertices() const { return this->object_data_.num_vertices; }
  auto const& vertices() const { return this->object_data_.vertices; }
  auto const& indices() const { return this->object_data_.indices; }
};

template<typename V, std::size_t N>
class triangle : public shape {
  std::array<float, N> vertices_;

  explicit constexpr triangle(GLenum const dm, std::array<float, N> &&v)
    : shape(dm)
    , vertices_(MOVE(v))
  {}

  friend class triangle_factory;
public:
  static auto constexpr NUM_VERTICES = 3u;
  static auto constexpr NUM_FLOATS_PER_VERTEX = V::NUM_FLOATS_PER_VERTEX;

  auto num_vertices() const { return NUM_VERTICES; }
  auto const& vertices() const { return this->vertices_; }
  auto const& indices() const
  {
    static constexpr std::array<GLuint, NUM_VERTICES> INDICES = {{0, 1, 2}};
    return INDICES;
  }
};

template <typename V, std::size_t N>
class rectangle : public shape {
  std::array<float, N> vertices_;

  explicit constexpr rectangle(GLenum const dm, std::array<float, N> &&v)
      : shape(dm)
      , vertices_(MOVE(v))
  {
  }

  friend class rectangle_factory;
public:
  static auto constexpr NUM_VERTICES = 6u;
  static auto constexpr NUM_FLOATS_PER_VERTEX = V::NUM_FLOATS_PER_VERTEX;

  auto num_vertices() const { return NUM_VERTICES; }
  auto const& vertices() const { return this->vertices_; }
  auto const& indices() const
  {
    static constexpr std::array<GLuint, NUM_VERTICES> INDICES = {{0, 1, 2, 3, 4, 5}};
    return INDICES;
  }
};

template <typename V>
class polygon : public shape {
  unsigned int const num_vertices_;
  stlw::sized_buffer<float> vertices_;

  // TODO: This is super wasteful.. we only need to yield an integer sequence.
  stlw::sized_buffer<GLuint> indices_;

  explicit polygon(GLenum const dm, unsigned int num_vertices, unsigned int const num_floats)
      : shape(dm)
      , num_vertices_(num_vertices)
      , vertices_(num_floats)
      , indices_(num_vertices)
  {
    assert((num_floats % num_vertices) == 0);

    // TODO: wasteful
    FOR(i, num_vertices) {
      indices_[i] = i;
    }
  }

  friend class polygon_factory;
public:
  static auto constexpr NUM_FLOATS_PER_VERTEX = V::NUM_FLOATS_PER_VERTEX;

  auto num_vertices() const { return this->num_vertices_; }

  auto const& vertices() const { return this->vertices_; }
  auto& vertices() { return this->vertices_; }

  auto const& indices() const { return this->indices_; }
};

template <typename V, std::size_t N>
class cube : public shape {
  std::array<float, N> vertices_;

  friend class cube_factory;
public:
  static auto constexpr NUM_VERTICES = 8;

  explicit constexpr cube(GLenum const dm, std::array<float, N> &&v)
      : shape(dm)
      , vertices_(MOVE(v))
  {
  }

  auto num_vertices() const { return NUM_VERTICES; }
  auto const& vertices() const { return this->vertices_; }
  auto const& indices() const
  {
    // clang-format off
    static constexpr std::array<GLuint, 14> INDICES = {{
      3, 2, 6, 7, 4, 2, 0,
      3, 1, 6, 5, 4, 1, 0
    }};
    // clang-format on
    return INDICES;
  }
};

} // ns opengl
