#pragma once
#include <boomhs/obj.hpp>
#include <opengl/buffer.hpp>
#include <opengl/vertex_attribute.hpp>
#include <stlw/log.hpp>

#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace boomhs
{

struct ObjQuery
{
  std::string               name;
  opengl::BufferFlags const flags;
};

bool
operator==(ObjQuery const&, ObjQuery const&);

bool
operator!=(ObjQuery const&, ObjQuery const&);

std::ostream&
operator<<(std::ostream&, ObjQuery const&);

/*
class ObjStore;
class ObjCache
{
  using pair_t        = std::pair<ObjQuery, opengl::VertexBuffer>;
  using VertexBuffers = std::vector<pair_t>;

  mutable VertexBuffers buffers_;

  // ObjCache should only be constructed by the ObjStore.
  friend class ObjStore;
  friend std::ostream& operator<<(std::ostream&, ObjCache const&);

  ObjCache() = default;

  void insert_buffer(pair_t&&) const;

  bool has_obj(ObjQuery const&) const;

  opengl::VertexBuffer const& get_obj(stlw::Logger&, ObjQuery const&) const;

  auto size() const { return buffers_.size(); }
  bool empty() const { return buffers_.empty(); }
};

std::ostream&
operator<<(std::ostream&, ObjCache const&);
*/

class ObjStore
{
  using pair_t      = std::pair<std::string, ObjData>;
  using datastore_t = std::vector<pair_t>;

  // This holds the data
  mutable datastore_t data_;

public:
  ObjStore() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjStore);

  void add_obj(std::string const&, ObjData&&) const;

  ObjData&       get(stlw::Logger&, std::string const&);
  ObjData const& get(stlw::Logger&, std::string const&) const;

  auto size() const { return data_.size(); }
  bool empty() const { return data_.empty(); }
};

std::ostream&
operator<<(std::ostream&, ObjStore const&);

/*
class InterleaveCache
{
  ObjCache pos_;
  ObjCache pos_color_;
  ObjCache pos_normal_;
  ObjCache pos_color_normal_;
  ObjCache pos_normal_uv_;
  ObjCache pos_uv_;

  ObjCache& find_cache(stlw::Logger&, ObjQuery const&);
  ObjCache const& find_cache(stlw::Logger&, ObjQuery const&) const;

public:
  void add_obj(std::string const&, ObjData&&) const;
  ObjData const& data_for(stlw::Logger&, ObjQuery const&) const;

  opengl::VertexBuffer const& get_obj(stlw::Logger&, ObjQuery const&) const;
  opengl::VertexBuffer        get_copy(stlw::Logger&, ObjQuery const&) const;

  auto size() const { return data_.size(); }
  bool empty() const { return data_.empty(); }
};

std::ostream&
operator<<(std::ostream&, InterleaveCache const&);
*/

} // namespace boomhs
