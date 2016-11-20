#pragma once

namespace stlw
{

// Structure abstracting over a pointer and a length.
template<typename T>
class slice {
  T const* data_;
  std::size_t const length_;

  explicit constexpr slice(T const* d, std::size_t const l)
    : data_(d)
    , length_(l)
  {}

  template<typename X>
  friend constexpr slice<X> make_slice(X const*, std::size_t const);
public:
  inline constexpr auto *const data() const { return this->data_; }
  inline constexpr auto const length() const { return this->length_; }
  inline constexpr T const& operator[](std::size_t const l) const { return *(this->data() + l); }
};

template<typename T>
constexpr slice<T>
make_slice(T const* data, std::size_t const length)
{
  return slice<T>{data, length};
}

} // ns stlw
