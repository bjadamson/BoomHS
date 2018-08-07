#pragma once
#include <opengl/bind.hpp>
#include <opengl/global.hpp>
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
  DebugBoundCheck debug_check;

  NO_COPY(DrawInfo);
  explicit DrawInfo(size_t, GLuint);

  DrawInfo(DrawInfo&&);
  DrawInfo& operator=(DrawInfo&&);

  void bind_impl(stlw::Logger&);
  void unbind_impl(stlw::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  auto vbo() const { return handles_.vbo(); }
  auto ebo() const { return handles_.ebo(); }
  auto num_vertexes() const { return num_vertexes_; }
  auto num_indices() const { return num_indices_; }

  auto&       vao() { return vao_; }
  auto const& vao() const { return vao_; }

  std::string to_string() const;
};

class DrawHandleManager;
class EntityDrawHandleMap
{
  std::vector<opengl::DrawInfo> drawinfos_;
  std::vector<boomhs::EntityID> entities_;

  friend class DrawHandleManager;

  EntityDrawHandleMap() = default;
  NO_COPY(EntityDrawHandleMap);
  MOVE_DEFAULT(EntityDrawHandleMap);

  size_t add(boomhs::EntityID, opengl::DrawInfo&&);

  bool empty() const { return drawinfos_.empty(); }

  opengl::DrawInfo*       lookup(stlw::Logger&, boomhs::EntityID);
  opengl::DrawInfo const* lookup(stlw::Logger&, boomhs::EntityID) const;
};

class DrawHandleManager
{
  // These slots get a value when memory is loaded, set to none when memory is not.
  EntityDrawHandleMap entities_;
  EntityDrawHandleMap bboxes_;

public:
  DrawHandleManager() = default;
  NO_COPY(DrawHandleManager);
  MOVE_DEFAULT(DrawHandleManager);

  void add_entity(boomhs::EntityID, DrawInfo&&);
  void add_bbox(boomhs::EntityID, DrawInfo&&);

  EntityDrawHandleMap&       entities();
  EntityDrawHandleMap const& entities() const;

  EntityDrawHandleMap&       bbox_entities();
  EntityDrawHandleMap const& bbox_entities() const;

  DrawInfo&       lookup_entity(stlw::Logger&, boomhs::EntityID);
  DrawInfo const& lookup_entity(stlw::Logger&, boomhs::EntityID) const;

  DrawInfo&       lookup_bbox(stlw::Logger&, boomhs::EntityID);
  DrawInfo const& lookup_bbox(stlw::Logger&, boomhs::EntityID) const;
};

} // namespace opengl
