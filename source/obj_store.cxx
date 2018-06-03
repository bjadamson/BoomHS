#include <boomhs/obj_store.hpp>
#include <stlw/algorithm.hpp>

#include <algorithm>
#include <extlibs/fmt.hpp>
#include <iomanip>

using namespace opengl;

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// ObjQuery
bool
operator==(ObjQuery const& a, ObjQuery const& b)
{
  // clang-format off
  return ALLOF(
      a.name == b.name,
      a.flags == b.flags);
  // clang-format on
}

bool
operator!=(ObjQuery const& a, ObjQuery const& b)
{
  return !(a == b);
}

std::ostream&
operator<<(std::ostream& stream, ObjQuery const& query)
{
  stream << "{name: '";
  stream << std::setw(10) << query.name;
  stream << "', flags: '";
  stream << query.flags;
  stream << "'}";
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ObjCache
void
ObjCache::insert_buffer(ObjCache::pair_t&& pair) const
{
  buffers_.emplace_back(MOVE(pair));
}

#define FIND_OBJ_IN_CACHE(query, buffers)                                                          \
  [&]() {                                                                                          \
    auto const cmp = [&](auto const& pair) { return pair.first.name == query.name; };              \
    return std::find_if(buffers.cbegin(), buffers.cend(), cmp);                                    \
  }()

bool
ObjCache::has_obj(ObjQuery const& query) const
{
  return FIND_OBJ_IN_CACHE(query, buffers_) != buffers_.cend();
}

VertexBuffer const&
ObjCache::get_obj(stlw::Logger& logger, ObjQuery const& query) const
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
ObjStore::add_obj(std::string const& name, ObjData&& o) const
{
  auto pair = std::make_pair(name, MOVE(o));
  data_.emplace_back(MOVE(pair));
}

ObjData const&
ObjStore::data_for(stlw::Logger& logger, ObjQuery const& query) const
{
  auto const cmp = [&query](auto const& pair) {
    bool const  names_match    = pair.first == query.name;
    auto const& data           = pair.second;
    bool const  vertices_empty = data.vertices.empty();
    bool const  colors_empty   = data.colors.empty();
    bool const  normals_empty  = data.normals.empty();
    bool const  uvs_empty      = data.uvs.empty();

    // FOR NOW, we assume all flags present in .obj file
    assert(ALLOF(!vertices_empty, !colors_empty, !normals_empty, !uvs_empty));
    return names_match;
  };
  auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);

  // Assume the datastore has the object, somewhere
  LOG_TRACE_SPRINTF("Looking for object '%s'", query.name);
  assert(it != data_.cend());
  return it->second;
}

VertexBuffer const&
ObjStore::get_obj(stlw::Logger& logger, ObjQuery const& query) const
{
  auto const& cache         = find_cache(logger, query);
  bool const  cache_has_obj = cache.has_obj(query);
  if (cache_has_obj) {
    return cache.get_obj(logger, query);
  }

  {
    // We need to read data from the ObjStore to construct an instance to put into the cache.
    auto const& data = data_for(logger, query);

    auto const& query_attr = query.flags;
    auto        buffer     = VertexBuffer::create_interleaved(logger, data, query_attr);
    auto        pair       = std::make_pair(query, MOVE(buffer));
    cache.insert_buffer(MOVE(pair));
  }

  // yield reference to data
  assert(cache.has_obj(query));
  return cache.get_obj(logger, query);
}

VertexBuffer
ObjStore::get_copy(stlw::Logger& logger, ObjQuery const& query) const
{
  return get_obj(logger, query).copy();
}

#define FIND_CACHE(query, cache)                                                                   \
  auto const& flags            = query.flags;                                                      \
  bool const  pos_only         = ALLOF(flags.vertices, !flags.normals, !flags.colors, !flags.uvs); \
  bool const  pos_normal       = ALLOF(flags.vertices, flags.normals, !flags.colors, !flags.uvs);  \
  bool const  pos_color        = ALLOF(flags.vertices, !flags.normals, flags.colors, !flags.uvs);  \
  bool const  pos_color_normal = ALLOF(flags.vertices, flags.normals, flags.colors, !flags.uvs);   \
  bool const  pos_normal_uvs   = ALLOF(flags.vertices, flags.normals, !flags.colors, flags.uvs);   \
                                                                                                   \
  /* invalid configurations */                                                                     \
  bool const no_vertices   = ALLOF(!flags.vertices);                                               \
  bool const color_and_uvs = ALLOF(flags.colors, flags.uvs);                                       \
                                                                                                   \
  if (no_vertices) {                                                                               \
    LOG_ERROR("mesh: %s cannot be found (no vertices) not implemented.", query.name);              \
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
  else if (pos_color) {                                                                            \
    cache = &pos_color_;                                                                           \
  }                                                                                                \
  else if (pos_normal) {                                                                           \
    cache = &pos_normal_;                                                                          \
  }                                                                                                \
  else if (pos_color_normal) {                                                                     \
    cache = &pos_color_normal_;                                                                    \
  }                                                                                                \
  else if (flags.normals && flags.uvs) {                                                           \
    cache = &pos_normal_uv_;                                                                       \
  }                                                                                                \
  else if (flags.uvs) {                                                                            \
    cache = &pos_uv_;                                                                              \
  }                                                                                                \
  else {                                                                                           \
    LOG_ERROR("invalid query: %s");                                                                \
    std::abort();                                                                                  \
  }                                                                                                \
  assert(nullptr != cache);

ObjCache&
ObjStore::find_cache(stlw::Logger& logger, ObjQuery const& query)
{
  ObjCache* cache = nullptr;
  FIND_CACHE(query, cache);
  return *cache;
}

ObjCache const&
ObjStore::find_cache(stlw::Logger& logger, ObjQuery const& query) const
{
  ObjCache const* cache = nullptr;
  FIND_CACHE(query, cache);
  return *cache;
}

#undef FIND_CACHE

std::ostream&
operator<<(std::ostream& stream, ObjCache const& cache)
{
  auto const& buffers = cache.buffers_;
  auto const  WS      = "    ";

  stream << "{";
  stream << "(cache SIZE: " << cache.size() << ") ";
  if (!cache.empty()) {
    stream << "\n";
    stream << WS;
  }
  FOR(i, buffers.size())
  {
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
operator<<(std::ostream& stream, ObjStore const& store)
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

} // namespace boomhs
