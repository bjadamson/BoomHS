#include <boomhs/obj_store.hpp>
#include <common/algorithm.hpp>

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
  return common::and_all(
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
// ObjStore
void
ObjStore::add_obj(std::string const& name, ObjData&& o) const
{
  auto pair = std::make_pair(name, MOVE(o));
  data_.emplace_back(MOVE(pair));
}

#define OBJSTORE_FIND(BEGIN, END)                                                                  \
  auto const cmp = [&](auto const& pair) { return pair.first == name; };                           \
  auto const it  = std::find_if(BEGIN, END, cmp);                                                  \
  assert(it != END);                                                                               \
  return it->second;

ObjData&
ObjStore::get(common::Logger& logger, std::string const& name)
{
  OBJSTORE_FIND(data_.begin(), data_.end());
}

ObjData const&
ObjStore::get(common::Logger& logger, std::string const& name) const
{
  OBJSTORE_FIND(data_.cbegin(), data_.cend());
}

#undef OBJSTORE_FIND

} // namespace boomhs
