#pragma once
#include <opengl/types.hpp>
#include <opengl/shape.hpp>
#include <stlw/sized_buffer.hpp>

namespace opengl
{

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

} // ns opengl
