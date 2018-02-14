#include <boomhs/assets.hpp>
#include <type_traits>

using namespace boomhs;

namespace
{

// This macro exists to reduce code duplication implementingt he two different implementation of
// operator[].
#define SEARCH_FOR(type, begin, end)                                                               \
  auto const cmp = [&type](auto const& tinfo) {                                                    \
    return tinfo.type == type;                                                                     \
  };                                                                                               \
  auto const it = std::find_if(begin, end, cmp);                                                   \
  assert(it != end);                                                                               \
  return *it;

} // ns anon

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TileInfos
TileInfo const&
TileInfos::operator[](TileType const type) const
{
  SEARCH_FOR(type, data_.cbegin(), data_.cend());
}

TileInfo&
TileInfos::operator[](TileType const type)
{
  SEARCH_FOR(type, data_.begin(), data_.end());
}
#undef SEARCH_FOR

////////////////////////////////////////////////////////////////////////////////////////////////////
// ObjCache
void
ObjCache::add_obj(std::string const& name, opengl::obj &&o)
{
  auto pair = std::make_pair(name, MOVE(o));
  objects_.emplace_back(MOVE(pair));
}

void
ObjCache::add_obj(char const* name, opengl::obj &&o)
{
  add_obj(std::string{name}, MOVE(o));
}

opengl::obj const&
ObjCache::get_obj(char const* name) const
{
  auto const cmp = [&name](auto const& pair) {
    return pair.first == name;
  };
  auto const it = std::find_if(objects_.cbegin(), objects_.cend(), cmp);

  // assume presence
  assert(it != objects_.cend());

  // yield reference to data
  return it->second;
}

opengl::obj const&
ObjCache::get_obj(std::string const& s) const
{
  return get_obj(s.c_str());
}

} // ns boomhs
