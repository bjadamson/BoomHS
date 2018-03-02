#include <boomhs/obj_store.hpp>
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
ObjCache::insert_buffer(ObjCache::pair_t &&pair) const
{
  buffers_.emplace_back(MOVE(pair));
}

#define FIND_OBJ_IN_CACHE(query, buffers)                                                          \
  [&]()                                                                                            \
{                                                                                                  \
  auto const cmp = [&](auto const& pair) { return pair.first.name == query.name; };                \
  return std::find_if(buffers.cbegin(), buffers.cend(), cmp);                                      \
}()

bool
ObjCache::has_obj(ObjQuery const& query) const
{
  assert(objstore_);
  return FIND_OBJ_IN_CACHE(query, buffers_) != buffers_.cend();
}

ObjBuffer const&
ObjCache::get_obj(ObjQuery const& query) const
{
  {
    assert(objstore_);
    auto const it = FIND_OBJ_IN_CACHE(query, buffers_);
    if (it != buffers_.cend()) {
      return it->second;
    }
  }

  // We need to read data from the ObjStore to construct an instance to put into the cache.
  auto const& data = objstore_->data_for(query);
  auto const num_vertices = data.num_vertices;

  ObjBuffer buffer;
  auto &v = buffer.vertices;
  {
    size_t index = 0;
    FOR(i, num_vertices) {
      {
        auto const& p = data.positions;
        v.emplace_back(p[index++]);
        v.emplace_back(p[index++]);
        v.emplace_back(p[index++]);
        v.emplace_back(p[index++]);
      }
      if (!data.colors.empty()) {
        // encode assumptions for now
        assert(data.uvs.empty());

        auto const& c = data.colors;
        v.emplace_back(c[index++]);
        v.emplace_back(c[index++]);
        v.emplace_back(c[index++]);
        v.emplace_back(c[index++]);
      }
      if (!data.normals.empty()) {
        auto const& n = data.normals;
        v.emplace_back(n[index++]);
        v.emplace_back(n[index++]);
        v.emplace_back(n[index++]);
      }
      if (!data.uvs.empty()) {
        // encode assumptions for now
        assert(data.colors.empty());

        auto const& n = data.uvs;
        v.emplace_back(n[index++]);
        v.emplace_back(n[index++]);
      }
    }
  }
  FOR(i, buffer.indices.size()) {
    v.emplace_back(buffer.indices[i]);
  }

  auto pair = std::make_pair(query, MOVE(buffer));
  insert_buffer(MOVE(pair));

  auto const it = FIND_OBJ_IN_CACHE(query, buffers_);
  if (it == buffers_.cend()) {
    std::cerr << "OBJ not in cache immediately after insertion.\n";
    std::abort();
  }
  // We can give a reference away to it now.
  return it->second;
}
#undef FIND_OBJ_IN_CACHE

void
ObjCache::set_objstore(ObjStore &store)
{
  objstore_ = &store;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ObjStore
ObjStore::ObjStore()
{
  // TODO: somehow automate this ...?
  pos_.set_objstore(*this);
  pos_color_normal_.set_objstore(*this);
  pos_uv_.set_objstore(*this);
}

void
ObjStore::add_obj(ObjQuery const& query, ObjData &&o)
{
  auto pair = std::make_pair(query.name, MOVE(o));

  auto &cache = find_cache(query);
  data_.emplace_back(MOVE(pair));
}


ObjData const&
ObjStore::data_for(ObjQuery const& query) const
{
  auto const cmp = [&query](auto const& pair) { return pair.first == query.name; };
  auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);

  // Assume the datastore has the object, somewhere
  assert(it != data_.cend());
  return it->second;
}

ObjBuffer const&
ObjStore::get_obj(ObjQuery const& query) const
{
  auto const& cache = find_cache(query);

  auto const cmp = [&query](auto const& pair) {
    return pair.first == query;
  };
  if (!cache.has_obj(query)) {
    std::cerr << "Could not find mesh with requested attributes:\n"
      "QueryObject: '" << query << "'\n";
    std::abort();
  }

  // yield reference to data
  return cache.get_obj(query);
}

#define FIND_CACHE(query, cache)                                                                   \
  auto const& attr = query.attributes;                                                             \
  if (!attr.positions) {                                                                           \
    std::cerr << "not implemented.\n";                                                             \
    std::abort();                                                                                  \
  }                                                                                                \
  else if (!attr.colors && !attr.normals && !attr.uvs) {                                           \
    cache = &pos_;                                                                                 \
  }                                                                                                \
  else if (attr.colors && attr.normals) {                                                          \
    cache = &pos_color_normal_;                                                                    \
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
    cache = &pos_uv_;                                                                              \
  }                                                                                                \
  else {                                                                                           \
    std::cerr << "invalid\n";                                                                      \
    std::abort();                                                                                  \
  }                                                                                                \
  assert(nullptr != cache);                                                                        \
  return *cache;

ObjCache&
ObjStore::find_cache(ObjQuery const& query)
{
  ObjCache *cache = nullptr;
  FIND_CACHE(query, cache);
  return *cache;
}

ObjCache const&
ObjStore::find_cache(ObjQuery const& query) const
{
  ObjCache const* cache = nullptr;
  FIND_CACHE(query, cache);
  return *cache;
}

#undef FIND_CACHE

} // ns boomhs
