#pragma once
#include <opengl/vao.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/entity.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <string>

namespace opengl
{

class BufferHandles
{
  GLuint vbo_ = 0, ebo_ = 0;
  static auto constexpr NUM_BUFFERS = 1;

  explicit BufferHandles();
  NO_COPY(BufferHandles);

public:
  friend class DrawInfo;
  ~BufferHandles();

  // move-construction OK.
  BufferHandles(BufferHandles&&);

  BufferHandles& operator=(BufferHandles&&);

  auto vbo() const { return vbo_; }
  auto ebo() const { return ebo_; }

  std::string to_string() const;
};

std::ostream&
operator<<(std::ostream&, BufferHandles const&);

class DrawInfo
{
  size_t        num_vertexes_;
  GLuint        num_indices_;
  BufferHandles handles_;
  VAO           vao_;

public:
  NO_COPY(DrawInfo);
  explicit DrawInfo(size_t, GLuint);

  DrawInfo(DrawInfo&&);
  DrawInfo& operator=(DrawInfo&&);

  auto  vbo() const { return handles_.vbo(); }
  auto  ebo() const { return handles_.ebo(); }
  auto  num_vertexes() const { return num_vertexes_; }
  auto  num_indices() const { return num_indices_; }
  auto& vao() { return vao_; }

  std::string to_string() const;
};

class EntityDrawHandleMap
{
  std::vector<opengl::DrawInfo> drawinfos_;
  std::vector<boomhs::EntityID> entities_;

public:
  EntityDrawHandleMap() = default;
  NO_COPY(EntityDrawHandleMap);
  MOVE_DEFAULT(EntityDrawHandleMap);

  size_t add(boomhs::EntityID, opengl::DrawInfo&&);

  bool empty() const { return drawinfos_.empty(); }

  opengl::DrawInfo&       lookup(stlw::Logger&, boomhs::EntityID);
  opengl::DrawInfo const& lookup(stlw::Logger&, boomhs::EntityID) const;
};

} // namespace opengl
