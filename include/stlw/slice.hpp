#pragma once

namespace stlw
{

// Structure abstracting over a pointer and a length.
template <typename T>
class slice
{
  size_t const length_;
  T const*     data_;

  explicit constexpr slice(size_t const l, T const* d)
      : length_(l)
      , data_(d)
  {
  }

  template <typename X>
  friend constexpr slice<X> make_slice(size_t const, X const*);

public:
  inline constexpr auto* const data() const { return this->data_; }
  inline constexpr auto const  length() const { return this->length_; }
  inline constexpr T const&    operator[](size_t const l) const { return *(this->data() + l); }
};

template <typename T>
constexpr slice<T>
make_slice(size_t const length, T const* data)
{
  return slice<T>{length, data};
}

} // namespace stlw
