#pragma once
#include <vector>
#include <stlw/type_macros.hpp>

namespace stlw
{

// Allocates immediatly upon construction.
template<typename T, typename A=std::allocator<T>>
class sized_buffer
{
  template<typename Tt, typename Aa>
  using BT = std::vector<Tt, Aa>;

  BT<T, A> vec_;

  // TODO: see if this is necessary or not.
  inline void reserve() { this->vec_.reserve(vec_.size()); }
public:
  sized_buffer(sized_buffer &&other) : vec_(std::move(other.vec_)) {}
  NO_MOVE_ASSIGN(sized_buffer);

  COPY_DEFAULT(sized_buffer);

  // clang-format off
  using value_type             = typename BT<T, A>::value_type;
  using allocator_type         = typename BT<T, A>::allocator_type;
  using size_type              = typename BT<T, A>::size_type;
  using difference_type        = typename BT<T, A>::difference_type;
  using reference              = typename BT<T, A>::reference;
  using const_reference        = typename BT<T, A>::const_reference;
  using pointer                = typename BT<T, A>::pointer;
  using const_pointer          = typename BT<T, A>::const_pointer;
  using iterator               = typename BT<T, A>::iterator;
  using const_iterator         = typename BT<T, A>::const_iterator;
  using reverse_iterator       = typename BT<T, A>::reverse_iterator;
  using const_reverse_iterator = typename BT<T, A>::const_reverse_iterator;
  // clang-format on

  // constructors
  sized_buffer(size_type const count, T const& value, A const& alloc = A{})
    : vec_(count, value, alloc)
    {
      reserve();
    }
  explicit sized_buffer(size_type const count)
    : vec_(count)
  {
    reserve();
  }

  template<typename InputIt>
  sized_buffer(InputIt first, InputIt last, A const& alloc = A{})
    : vec_(first, last, alloc)
    {
      reserve();
    }

  // methods
  reference       operator[](size_type const i) { return this->vec_[i]; }
  const_reference operator[](size_type const i) const { return this->vec_[i]; }

  size_type length() const { return this->vec_.capacity(); }
  T* data() { return this->vec_.data(); }
  T const* data() const { return this->vec_.data(); }
};

} // ns stlw
