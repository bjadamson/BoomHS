#pragma once
#include <boomhs/obj.hpp>
#include <opengl/vertex_attribute.hpp>
#include <stlw/log.hpp>

#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace boomhs
{

struct QueryAttributes
{
  bool positions = true;
  bool normals = true;
  bool colors = true;
  bool uvs = true;

  static QueryAttributes from_va(opengl::VertexAttribute const&);
};

bool
operator==(QueryAttributes const&, QueryAttributes const&);

bool
operator!=(QueryAttributes const&, QueryAttributes const&);

std::ostream&
operator<<(std::ostream&, QueryAttributes const&);

struct ObjQuery
{
  std::string     name;
  QueryAttributes attributes = {};
};

bool
operator==(ObjQuery const&, ObjQuery const&);

bool
operator!=(ObjQuery const&, ObjQuery const&);

std::ostream&
operator<<(std::ostream&, ObjQuery const&);

class ObjStore;
class ObjCache
{
  using pair_t = std::pair<ObjQuery, ObjBuffer>;
  using ObjBuffers = std::vector<pair_t>;

  mutable ObjBuffers buffers_;

  // ObjCache should only be constructed by the ObjStore.
  friend class ObjStore;
  friend std::ostream& operator<<(std::ostream&, ObjCache const&);

  ObjCache() = default;

  void insert_buffer(pair_t&&) const;

  bool has_obj(ObjQuery const&) const;

  ObjBuffer const& get_obj(stlw::Logger&, ObjQuery const&) const;

  auto size() const { return buffers_.size(); }
  bool empty() const { return buffers_.empty(); }
};

std::ostream&
operator<<(std::ostream&, ObjCache const&);

class ObjStore
{
  using pair_t = std::pair<std::string, ObjData>;
  using datastore_t = std::vector<pair_t>;

  // This holds the data
  mutable datastore_t data_;

  // These caches hold cached versions of the data interleaved.
  ObjCache pos_;
  ObjCache pos_normal_;
  ObjCache pos_color_normal_;
  ObjCache pos_normal_uv_;
  ObjCache pos_uv_;

  ObjBuffer create_interleaved_buffer(ObjQuery const&) const;

  ObjData const& data_for(ObjQuery const&) const;

  ObjCache& find_cache(stlw::Logger&, ObjQuery const&);

  ObjCache const& find_cache(stlw::Logger&, ObjQuery const&) const;

  friend class ObjCache;
  friend std::ostream& operator<<(std::ostream&, ObjStore const&);

public:
  ObjStore() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjStore);

  void add_obj(std::string const&, ObjData&&) const;

  ObjBuffer const& get_obj(stlw::Logger&, ObjQuery const&) const;

  auto size() const { return data_.size(); }
  bool empty() const { return data_.empty(); }
};

std::ostream&
operator<<(std::ostream&, ObjStore const&);

} // namespace boomhs
