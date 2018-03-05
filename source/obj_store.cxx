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
  return FIND_OBJ_IN_CACHE(query, buffers_) != buffers_.cend();
}

ObjBuffer const&
ObjCache::get_obj(ObjQuery const& query) const
{
  auto const it = FIND_OBJ_IN_CACHE(query, buffers_);
  if (it != buffers_.cend()) {
    return it->second;
  }
  std::abort();
}
#undef FIND_OBJ_IN_CACHE

////////////////////////////////////////////////////////////////////////////////////////////////////
// ObjStore
void
ObjStore::add_obj(ObjQuery const& query, ObjData &&o) const
{
  auto pair = std::make_pair(query.name, MOVE(o));
  data_.emplace_back(MOVE(pair));

  auto const& cache = find_cache(query);
  if (!cache.has_obj(query)) {
    auto buffer = create_interleaved_buffer(query);
    auto pair = std::make_pair(query, MOVE(buffer));
    cache.insert_buffer(MOVE(pair));

    // yield reference to data
    assert(cache.has_obj(query));
  }
}

ObjData const&
ObjStore::data_for(ObjQuery const& query) const
{
  auto const cmp = [&query](auto const& pair) {
    bool const names_match = pair.first == query.name;
    auto const& data = pair.second;
    bool const positions = data.positions.empty();
    bool const colors = data.colors.empty();
    bool const normals = data.normals.empty();
    bool const uvs = data.uvs.empty();
    return names_match && positions && colors && normals && uvs;
  };
  auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);

  // Assume the datastore has the object, somewhere
  std::cerr << "Error looking up query:\n" << query << "\n    in objstore:\n    " << *this << "\n";
  assert(it != data_.cend());
  return it->second;
}

ObjBuffer
ObjStore::create_interleaved_buffer(ObjQuery const& query) const
{
  // We need to read data from the ObjStore to construct an instance to put into the cache.
  auto const& data = data_for(query);
  auto const num_vertices = data.num_vertices;

  ObjBuffer buffer;
  auto &v = buffer.vertices;
  auto &indices = buffer.indices;
  {
    size_t a = 0, b = 0, c = 0, d = 0;
    FOR(i, num_vertices) {
      {
        auto const& p = data.positions;
        v.emplace_back(p[a++]);
        v.emplace_back(p[a++]);
        v.emplace_back(p[a++]);
        v.emplace_back(p[a++]);
      }
      if (!data.colors.empty()) {
        // encode assumptions for now
        assert(data.uvs.empty());

        auto const& c = data.colors;
        v.emplace_back(c[b++]);
        v.emplace_back(c[b++]);
        v.emplace_back(c[b++]);
        v.emplace_back(c[b++]);
      }
      if (!data.normals.empty()) {
        auto const& n = data.normals;
        v.emplace_back(n[c++]);
        v.emplace_back(n[c++]);
        v.emplace_back(n[c++]);
        }
        if (!data.uvs.empty()) {
          // encode assumptions for now
          assert(data.colors.empty());

          auto const& n = data.uvs;
          v.emplace_back(n[d++]);
          v.emplace_back(n[d++]);
        }
    }
  }
  FOR(i, buffer.indices.size()) {
    indices.emplace_back(buffer.indices[i]);
  }

  return buffer;
}

ObjBuffer const&
ObjStore::get_obj(ObjQuery const& query) const
{
  auto const& cache = find_cache(query);
  bool const cache_has_obj = cache.has_obj(query);
  if (cache_has_obj) {
    return cache.get_obj(query);
  }
  std::abort();
}

#define FIND_CACHE(query, cache)                                                                   \
  auto const& attr = query.attributes;                                                             \
  if (!attr.positions) {                                                                           \
    std::cerr << "mesh: '" << query.name << "' cant find (no positions) not implemented.\n";       \
    std::abort();                                                                                  \
  }                                                                                                \
  else if (attr.normals) {                                                                         \
    cache = &pos_normal_;                                                                          \
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
    cache = &pos_normal_uv_;                                                                       \
  }                                                                                                \
  else if (attr.uvs) {                                                                             \
    cache = &pos_uv_;                                                                              \
  }                                                                                                \
  else {                                                                                           \
    std::cerr << "invalid query: '" << query << "'\n";                                             \
    std::abort();                                                                                  \
  }                                                                                                \
  assert(nullptr != cache);

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

std::ostream&
operator<<(std::ostream &stream, ObjCache const& cache)
{
  auto const& buffers = cache.buffers_;

  stream << "{";
  stream << "(cache SIZE: " << cache.size() << ") ";
  FOR(i, buffers.size()) {
    if (i > 0) {
      stream << ", ";
    }
    auto const& it = buffers[i];
    stream << it.first;
  }
  stream << "}";
  return stream;
}

std::ostream&
operator<<(std::ostream &stream, ObjStore const& store)
{
  stream << "{";
  stream << "pos_: {" << store.pos_ << "}";
  stream << ", pos_normal_: {" << store.pos_normal_ << "}";
  stream << ", pos_color_normal_: {" << store.pos_color_normal_ << "}";
  stream << ", pos_normal_uv_: {" << store.pos_normal_uv_ << "}";
  stream << ", pos_uv_: {" << store.pos_uv_ << "}";
  stream << "}";
  return stream;
}

} // ns boomhs
