#include <boomhs/gcd.hpp>

using namespace window;

namespace boomhs
{

void
GCD::reset_ms(ticks_t const t)
{
  timer_.set_ms(t);
}

} // namespace boomhs
