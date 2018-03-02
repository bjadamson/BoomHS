#include <boomhs/obj_cache.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/format.hpp>

#include <algorithm>
#include <iostream>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// QueryAttributes
bool
operator==(QueryAttributes const& a, QueryAttributes const& b)
{
  // clang-format off
  return ALLOF(
    a.positions == b.positions,
    b.colors == a.colors,
    a.uvs == b.uvs);
  // clang-format on
}

bool
operator!=(QueryAttributes const& a, QueryAttributes const& b)
{
  return !(a == b);
}

std::ostream&
operator<<(std::ostream &stream, QueryAttributes const& qa)
{
  stream << "{positions: '";
  stream << std::boolalpha << qa.positions;
  stream << "', colors: '";
  stream << std::boolalpha << qa.colors;
  stream << "', normals: '";
  stream << std::boolalpha << qa.normals;
  stream << "', uvs: '";
  stream << std::boolalpha << qa.uvs;
  stream << "'}";
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ObjQuery
bool
operator==(ObjQuery const& a, ObjQuery const& b)
{
  // clang-format off
  return ALLOF(
      a.name == b.name,
      a.attributes == b.attributes);
  // clang-format on
}

bool
operator!=(ObjQuery const& a, ObjQuery const& b)
{
  return !(a == b);
}

std::ostream&
operator<<(std::ostream & stream, ObjQuery const& query)
{
  stream << "{name: '";
  stream << query.name;
  stream << "', attributes: '";
  stream << query.attributes;
  stream << "'}";
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ObjCache
void
ObjCache::add_obj(ObjQuery const& query, Obj &&o)
{
  auto pair = std::make_pair(query, MOVE(o));

  auto &ds = find_ds(query);
  ds.emplace_back(MOVE(pair));
}

Obj const&
ObjCache::get_obj(ObjQuery const& query) const
{
  auto const& ds = find_ds(query);

  auto const cmp = [&query](auto const& pair) {
    return pair.first == query;
  };
  auto const it = std::find_if(ds.cbegin(), ds.cend(), cmp);

  // assume presence
  if (it == ds.cend()) {
    std::cerr << "Could not find mesh with requested attributes:\n"
      "QueryObject: '" << query << "'\n";
    std::abort();
  }

  // yield reference to data
  return it->second;
}

#define FIND_DS(qo, pds)                                                                           \
  auto const& attr = query.attributes;                                                             \
  if (!attr.positions) {                                                                           \
    std::cerr << "not implemented.\n";                                                             \
    std::abort();                                                                                  \
  }                                                                                                \
  else if (!attr.colors && !attr.normals && !attr.uvs) {                                           \
    pds = &pos_;                                                                                   \
  }                                                                                                \
  else if (attr.colors && attr.normals) {                                                          \
    pds = &pos_color_normal_;                                                                      \
  }                                                                                                \
  else if (attr.colors && attr.uvs) {                                                              \
    std::cerr << "invalid?\n";                                                                     \
    std::abort();                                                                                  \
  }                                                                                                \
  else if (attr.normals && attr.uvs) {                                                             \
    std::cerr << "not implemented.\n";                                                             \
    std::abort();                                                                                  \
  }                                                                                                \
  else if (attr.uvs) {                                                                             \
    pds = &pos_uv_;                                                                                \
  }                                                                                                \
  else {                                                                                           \
    std::cerr << "invalid\n";                                                                      \
    std::abort();                                                                                  \
  }                                                                                                \
  assert(nullptr != pds);                                                                          \
  return *pds;

ObjCache::datastore_t&
ObjCache::find_ds(ObjQuery const& query)
{
  datastore_t *pds = nullptr;
  FIND_DS(query, pds);
  return *pds;
}

ObjCache::datastore_t const&
ObjCache::find_ds(ObjQuery const& query) const
{
  datastore_t const* pds = nullptr;
  FIND_DS(query, pds);
  return *pds;
}

#undef FIND_DS

} // ns boomhs
