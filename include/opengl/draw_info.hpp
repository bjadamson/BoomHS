#pragma once
#include <opengl/bind.hpp>
#include <opengl/global.hpp>
#include <opengl/vao.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/entity.hpp>

#include <common/log.hpp>
#include <common/type_macros.hpp>

#include <optional>
#include <string>

namespace boomhs
{
class ObjStore;
} // namespace boomhs

namespace opengl
{
class ShaderPrograms;

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

  void bind_impl(common::Logger&);
  void unbind_impl(common::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  auto vbo() const { return handles_.vbo(); }
  auto ebo() const { return handles_.ebo(); }
  auto num_vertexes() const { return num_vertexes_; }
  auto num_indices() const { return num_indices_; }

  auto&       vao() { return vao_; }
  auto const& vao() const { return vao_; }

  std::string to_string() const;
};

struct DrawInfoHandle
{
  using value_type = size_t;

  value_type value;

  explicit DrawInfoHandle(value_type const v) : value(v) {}
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

  DrawInfoHandle add(boomhs::EntityID, opengl::DrawInfo&&);

  bool empty() const { return drawinfos_.empty(); }
  bool has(DrawInfoHandle) const;
  auto size() const { assert(drawinfos_.size() == entities_.size()); return drawinfos_.size(); }

  DrawInfo const& get(DrawInfoHandle) const;
  DrawInfo& get(DrawInfoHandle);

  std::optional<DrawInfoHandle> find(boomhs::EntityID) const;

};

class DrawHandleManager
{
  // These slots get a value when memory is loaded, set to none when memory is not.
  EntityDrawHandleMap entities_;

  EntityDrawHandleMap&       entities();
  EntityDrawHandleMap const& entities() const;

public:
  DrawHandleManager() = default;
  NO_COPY(DrawHandleManager);
  MOVE_DEFAULT(DrawHandleManager);

  // methods
  DrawInfoHandle add_entity(boomhs::EntityID, DrawInfo&&);

  DrawInfo&       lookup_entity(common::Logger&, boomhs::EntityID);
  DrawInfo const& lookup_entity(common::Logger&, boomhs::EntityID) const;

  void add_mesh(common::Logger&, ShaderPrograms&, boomhs::ObjStore&, boomhs::EntityID,
                boomhs::EntityRegistry&);
  void add_cube(common::Logger&, ShaderPrograms&, boomhs::EntityID, boomhs::EntityRegistry&);
};

} // namespace opengl
