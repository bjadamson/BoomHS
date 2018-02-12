#include <boomhs/assets.hpp>
#include <type_traits>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TileInfos
TileInfo const&
TileInfos::operator[](TileType const type) const
{
  auto const cmp = [&type](auto const& tinfo) {
    return tinfo.type == type;
  };
  auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);
  // assume presence
  assert(it != data_.cend());
  return *it;
}

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// GpuHandleList
size_t
GpuHandleList::add(uint32_t const entity, opengl::DrawInfo &&di)
{
  auto const pos = drawinfos_.size();
  drawinfos_.emplace_back(MOVE(di));
  entities_.emplace_back(entity);

  // return the index di was stored in.
  return pos;
}

opengl::DrawInfo const&
GpuHandleList::get(uint32_t const entity) const
{
  FOR(i, entities_.size()) {
    if (entities_[i] == entity) {
      return drawinfos_[i];
    }
  }
  std::cerr << fmt::format("Error could not find gpu handle associated to entity {}'\n", entity);
  std::abort();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GpuHandleManager
opengl::DrawInfo const&
HandleManager::lookup(uint32_t const eid) const
{
  return list_.get(eid);
}

} // ns boomhs
