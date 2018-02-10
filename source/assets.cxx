#include <boomhs/assets.hpp>

namespace boomhs
{

Assets::Assets(ObjCache &&o, LoadedEntities &&l, opengl::TextureTable &&tt, opengl::GlobalLight &&gl,
    opengl::Color &&c)
  : obj_cache(MOVE(o))
  , loaded_entities(MOVE(l))
  , texture_table(MOVE(tt))
  , global_light(MOVE(gl))
  , background_color(MOVE(c))
{
}

Assets::Assets(Assets &&other)
  : obj_cache(MOVE(other.obj_cache))
  , loaded_entities(MOVE(other.loaded_entities))
  , texture_table(MOVE(other.texture_table))
  , global_light(MOVE(other.global_light))
  , background_color(other.background_color)
{
  std::cerr << "moving assets\n";
  assert(!loaded_entities.empty());
  assert(other.loaded_entities.empty());
}

Assets::~Assets()
{
  if (!loaded_entities.empty()) {
    std::abort();
  }
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

  // for now, assume all queries are found
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
// LoadedEntities
LoadedEntities::LoadedEntities(LoadedEntities &&other)
    : data(MOVE(other.data))
{
  std::cerr << "MOVING LoadedEntities\n";
  assert(!data.empty());
  assert(other.data.empty());
}

LoadedEntities::~LoadedEntities()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GpuHandleList
GpuHandleList::~GpuHandleList()
{
  if (!drawinfos_.empty() || !entities_.empty()) {
    std::abort();
  }
}

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
HandleManager::~HandleManager()
{
  if (!list_.empty()) {
    std::abort();
  }
}

opengl::DrawInfo const&
HandleManager::lookup(uint32_t const eid) const
{
  return list_.get(eid);
}

} // ns boomhs
