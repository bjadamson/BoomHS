#include <boomhs/item.hpp>

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// PreviousOwners
void
PreviousOwners::add(char const* v)
{
  add(Name{v});
}

void
PreviousOwners::add(std::string const& v)
{
  add(Name{v});
}

void
PreviousOwners::add(Name const& v)
{
  if (!contains(v.value)) {
    values_.emplace_back(v);
  }
}

bool
PreviousOwners::contains(std::string const& v) const
{
  return contains(v.c_str());
}

bool
PreviousOwners::contains(char const* v) const
{
  // return values_.cend() != std::find(values.cbegin(), values_.cend(), Name{v});
  bool found = false;
  for (auto const& it : values_) {
    if (it.value == v) {
      found = true;
      break;
    }
  }
  return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Item
bool
Item::ever_owned_by(char const* v) const
{
  return owners_.contains(v);
}

bool
Item::was_previously_owned() const
{
  return owners_.size() > 1;
}

} // namespace boomhs
