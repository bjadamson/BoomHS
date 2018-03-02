#pragma once
#include <boomhs/obj.hpp>

#include <ostream>
#include <string>
#include <vector>
#include <utility>

namespace boomhs
{

struct QueryAttributes
{
  bool positions = true;
  bool colors = false;
  bool normals = false;
  bool uvs = false;
};

bool
operator==(QueryAttributes const&, QueryAttributes const&);

bool
operator!=(QueryAttributes const&, QueryAttributes const&);

std::ostream&
operator<<(std::ostream &, QueryAttributes const&);

struct ObjQuery
{
  std::string name;
  QueryAttributes attributes = {};
};

bool
operator==(ObjQuery const&, ObjQuery const&);

bool
operator!=(ObjQuery const&, ObjQuery const&);

std::ostream&
operator<<(std::ostream &, ObjQuery const&);

class ObjStore;
class ObjCache
{
  using pair_t = std::pair<ObjQuery, ObjBuffer>;
  using ObjBuffers = std::vector<pair_t>;

  ObjStore *objstore_ = nullptr;
  mutable ObjBuffers buffers_;

  // ObjCache should only be constructed by the ObjStore.
  friend class ObjStore;
  ObjCache() = default;

  void
  insert_buffer(pair_t &&) const;

  bool
  has_obj(ObjQuery const&) const;

  ObjBuffer const&
  get_obj(ObjQuery const&) const;

  void
  set_objstore(ObjStore &);
};

class ObjStore
{
  using pair_t = std::pair<std::string, ObjData>;
  using datastore_t = std::vector<pair_t>;

  // This holds the data
  datastore_t data_;

  // These caches hold cached versions of the data interleaved.
  ObjCache pos_;
  ObjCache pos_color_normal_;
  ObjCache pos_uv_;

  ObjData const&
  data_for(ObjQuery const&) const;

  ObjCache&
  find_cache(ObjQuery const&);

  ObjCache const&
  find_cache(ObjQuery const&) const;

  friend class ObjCache;
public:
  ObjStore();
  MOVE_CONSTRUCTIBLE_ONLY(ObjStore);

  void
  add_obj(ObjQuery const&, ObjData &&);

  ObjBuffer const&
  get_obj(ObjQuery const&) const;
};

} // ns boomhs
