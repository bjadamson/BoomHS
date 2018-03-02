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

class ObjStore
{
  using pair_t = std::pair<ObjQuery, ObjBuffer>;
  using datastore_t = std::vector<pair_t>;

  datastore_t pos_;
  datastore_t pos_color_normal_;
  datastore_t pos_uv_;

  datastore_t&
  find_ds(ObjQuery const&);

  datastore_t const&
  find_ds(ObjQuery const&) const;

public:
  ObjStore() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjStore);

  void
  add_obj(ObjQuery const&, ObjBuffer &&);

  ObjBuffer const&
  get_obj(ObjQuery const&) const;
};

} // ns boomhs
