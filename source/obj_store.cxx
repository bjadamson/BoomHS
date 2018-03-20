#include <boomhs/obj_store.hpp>
#include <stlw/algorithm.hpp>

#include <extlibs/fmt.hpp>
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// QueryAttributes
QueryAttributes
QueryAttributes::from_va(opengl::VertexAttribute const& va)
{
  bool const p = va.has_positions();
  bool const n = va.has_normals();
  bool const c = va.has_colors();
  bool const u = va.has_uvs();
  return QueryAttributes{p, n, c, u};
}

bool
operator==(QueryAttributes const& a, QueryAttributes const& b)
{
  // clang-format off
  return ALLOF(
    a.positions == b.positions,
    a.normals == b.normals,
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
  // 5 == std::strlen("false");
  static int constexpr MAX_LENGTH = 5;

  auto const print_bool = [&stream](char const* text, bool const v) {
    stream << text;
    stream << ": '";
    stream << std::boolalpha << v;
    stream << "'";
  };

  stream << "{";
  print_bool("positions", qa.positions);
  stream << ", ";

  print_bool("colors", qa.colors);
  stream << ", ";

  print_bool("normals", qa.normals);
  stream << ", ";

  print_bool("uvs", qa.uvs);

  stream << "}";
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
  stream << std::setw(10) << query.name;
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
ObjCache::get_obj(stlw::Logger &logger, ObjQuery const& query) const
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
ObjStore::add_obj(std::string const& name, ObjData &&o) const
{
  auto pair = std::make_pair(name, MOVE(o));
  data_.emplace_back(MOVE(pair));
}

ObjData const&
ObjStore::data_for(ObjQuery const& query) const
{
  auto const cmp = [&query](auto const& pair) {
    bool const names_match = pair.first == query.name;
    auto const& data = pair.second;
    bool const positions_empty = data.positions.empty();
    bool const colors_empty    = data.colors.empty();
    bool const normals_empty   = data.normals.empty();
    bool const uvs_empty       = data.uvs.empty();

    // FOR NOW, we assume all attributes present in .obj file
    assert(ALLOF(!positions_empty, !colors_empty, !normals_empty, !uvs_empty));
    return names_match;
  };
  auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);

  // Assume the datastore has the object, somewhere
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
  auto &vertices = buffer.vertices;

  auto const copy_n = [&vertices](auto const& buffer, size_t const num, size_t &count, size_t &remaining) {
    FOR(i, num) {
      vertices.emplace_back(buffer[count++]);
      --remaining;
    }
  };
  auto num_positions = data.positions.size();
  auto num_normals = data.normals.size();
  auto num_colors = data.colors.size();
  auto num_uvs = data.uvs.size();

  auto const keep_going = [&]() {
    return ALLOF(num_positions > 0,
        num_normals > 0,
        num_colors > 0,
        num_uvs > 0
        );
  };

  auto const& query_attr = query.attributes;
  size_t a = 0, b = 0, c = 0, d = 0;
  while(keep_going()) {
    assert(!data.positions.empty());
    copy_n(data.positions, 4, a, num_positions);

    if (query_attr.normals) {
      copy_n(data.normals, 3, b, num_normals);
    }

    if (query_attr.colors) {
        // encode assumptions for now
      assert(!query_attr.uvs);
      copy_n(data.colors, 4, c, num_colors);
    }
    if (query_attr.uvs) {
      // encode assumptions for now
      assert(!query_attr.colors);

      copy_n(data.uvs, 2, d, num_uvs);
    }
  }

  assert(num_positions == 0 || num_positions == data.positions.size());
  assert(num_normals == 0 || num_normals == data.normals.size());
  assert(num_colors == 0 || num_colors == data.colors.size());
  assert(num_uvs == 0 || num_uvs == data.uvs.size());

  buffer.indices = data.indices;
  assert(buffer.indices.size() == data.indices.size());

  return buffer;
}

ObjBuffer const&
ObjStore::get_obj(stlw::Logger &logger, ObjQuery const& query) const
{
  auto const& cache = find_cache(logger, query);
  bool const cache_has_obj = cache.has_obj(query);
  if (cache_has_obj) {
    return cache.get_obj(logger, query);
  }

  auto buffer = create_interleaved_buffer(query);
  auto pair = std::make_pair(query, MOVE(buffer));
  cache.insert_buffer(MOVE(pair));

  // yield reference to data
  assert(cache.has_obj(query));
  return cache.get_obj(logger, query);
}

#define FIND_CACHE(query, cache)                                                                   \
  auto const& attr = query.attributes;                                                             \
  bool const pos_only         = ALLOF(attr.positions, !attr.normals, !attr.colors, !attr.uvs);     \
  bool const pos_normal       = ALLOF(attr.positions, attr.normals, !attr.colors, !attr.uvs);      \
  bool const pos_color        = ALLOF(attr.positions, !attr.normals, attr.colors, !attr.uvs);      \
  bool const pos_color_normal = ALLOF(attr.positions, attr.normals, attr.colors, !attr.uvs);       \
  bool const pos_normal_uvs   = ALLOF(attr.positions, attr.normals, !attr.colors, attr.uvs);       \
                                                                                                   \
  /* invalid configurations */                                                                     \
  bool const no_positions     = ALLOF(!attr.positions);                                            \
  bool const color_and_uvs    = ALLOF(attr.colors, attr.uvs);                                      \
                                                                                                   \
  if (no_positions) {                                                                              \
    LOG_ERROR("mesh: %s cannot be found (no positions) not implemented.", query.name);             \
    std::abort();                                                                                  \
  }                                                                                                \
  else if (color_and_uvs) {                                                                        \
    LOG_ERROR("invalid?");                                                                         \
    std::abort();                                                                                  \
  }                                                                                                \
                                                                                                   \
  if (pos_only) {                                                                                  \
    cache = &pos_;                                                                                 \
  }                                                                                                \
  else if (pos_normal) {                                                                           \
    cache = &pos_normal_;                                                                          \
  }                                                                                                \
  else if (pos_color_normal) {                                                                     \
    cache = &pos_color_normal_;                                                                    \
  }                                                                                                \
  else if (attr.normals && attr.uvs) {                                                             \
    cache = &pos_normal_uv_;                                                                       \
  }                                                                                                \
  else if (attr.uvs) {                                                                             \
    cache = &pos_uv_;                                                                              \
  }                                                                                                \
  else {                                                                                           \
    LOG_ERROR("invalid query: %s");                                                                \
    std::abort();                                                                                  \
  }                                                                                                \
  assert(nullptr != cache);

ObjCache&
ObjStore::find_cache(stlw::Logger &logger, ObjQuery const& query)
{
  ObjCache *cache = nullptr;
  FIND_CACHE(query, cache);
  return *cache;
}

ObjCache const&
ObjStore::find_cache(stlw::Logger &logger, ObjQuery const& query) const
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
  auto const WS = "    ";

  stream << "{";
  stream << "(cache SIZE: " << cache.size() << ") ";
  if (!cache.empty()) {
    stream << "\n";
    stream << WS;
  }
  FOR(i, buffers.size()) {
    if (i > 0) {
      stream << "\n";
      stream << WS;
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
  // clang-format off
  auto const print_cache = [&stream](char const* name, auto const& cache) {
    stream << name;
    stream << ":";
    stream << cache;
    stream << "\n";
  };

  stream << "{";
  stream << "(size: '" << store.size() << "')\n";
  stream << "(names: '";
  {
    bool print_comma = false;
    for (auto const& pair : store.data_) {
      if (!print_comma) {
        print_comma = true;
      }
      else {
        stream << ", ";
      }
      stream << pair.first << "'";
    }
    stream << "')\n";
  }
  print_cache("pos_", store.pos_);
  print_cache("pos_normal_", store.pos_normal_);
  print_cache("pos_color_normal_", store.pos_color_normal_);
  print_cache("pos_normal_uv_", store.pos_normal_uv_);
  print_cache("pos_uv_", store.pos_uv_);
  stream << "}\n";
  // clang-format on
  return stream;
}

} // ns boomhs
