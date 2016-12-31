#pragma once
#include <stlw/type_macros.hpp>

namespace gfx
{

template<typename B>
class context
{
  B &backend_;
public:
  explicit context(B &b) : backend_(b) {}
  MOVE_CONSTRUCTIBLE_ONLY(context);

  B& backend() { return this->backend_; }
};

template<typename B>
auto
make_context(B &backend)
{
  return context<B>{backend};
}

} // ns gfx
