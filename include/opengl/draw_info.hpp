#pragma once
#include <opengl/texture.hpp>
#include <opengl/vao.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/entity.hpp>
#include <boomhs/tile.hpp>

#include <stlw/log.hpp>
#include <stlw/optional.hpp>
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
  GLenum        draw_mode_;
  size_t        num_vertexes_;
  GLuint        num_indices_;
  BufferHandles handles_;
  VAO           vao_;

  std::optional<TextureInfo> texture_info_;

public:
  NO_COPY(DrawInfo);
  explicit DrawInfo(GLenum, size_t, GLuint, std::optional<TextureInfo> const&);

  DrawInfo(DrawInfo&&);
  DrawInfo& operator=(DrawInfo&&);

  auto  draw_mode() const { return draw_mode_; }
  auto  vbo() const { return handles_.vbo(); }
  auto  ebo() const { return handles_.ebo(); }
  auto  num_vertexes() const { return num_vertexes_; }
  auto  num_indices() const { return num_indices_; }
  auto& vao() { return vao_; }

  void        set_ti(TextureInfo&& ti) { texture_info_ = MOVE(ti); }
  auto&       texture_info() { return texture_info_; }
  auto const& texture_info() const { return texture_info_; }

  std::string to_string(VertexAttribute const&) const;
};

class EntityDrawinfos
{
  std::vector<opengl::DrawInfo> drawinfos_;
  std::vector<boomhs::EntityID> entities_;

public:
  EntityDrawinfos() = default;
  NO_COPY(EntityDrawinfos);
  MOVE_DEFAULT(EntityDrawinfos);

  size_t add(boomhs::EntityID, opengl::DrawInfo&&);

  bool empty() const { return drawinfos_.empty(); }

  opengl::DrawInfo&       get(stlw::Logger&, boomhs::EntityID);
  opengl::DrawInfo const& get(stlw::Logger&, boomhs::EntityID) const;
};

class EntityDrawHandles
{
  EntityDrawinfos infos_;

public:
  NO_COPY(EntityDrawHandles);
  MOVE_DEFAULT(EntityDrawHandles);
  explicit EntityDrawHandles(EntityDrawinfos&& list)
      : infos_(MOVE(list))
  {
  }

  opengl::DrawInfo&       lookup(stlw::Logger&, boomhs::EntityID);
  opengl::DrawInfo const& lookup(stlw::Logger&, boomhs::EntityID) const;
};

class TileDrawHandles
{
public:
  std::vector<opengl::DrawInfo> drawinfos_;

public:
  NO_COPY(TileDrawHandles);
  MOVE_DEFAULT(TileDrawHandles);
  TileDrawHandles(std::vector<opengl::DrawInfo>&& dh)
      : drawinfos_(MOVE(dh))
  {
  }
  opengl::DrawInfo&       lookup(stlw::Logger&, boomhs::TileType);
  opengl::DrawInfo const& lookup(stlw::Logger&, boomhs::TileType) const;
};

} // namespace opengl
