#pragma once
#include <stlw/type_macros.hpp>

namespace engine::gfx
{

enum class IMAGES {
  CONTAINER = 0,
  WALL
};

template<std::size_t N>
class table
{
public:
  using X = char const*;
private:
  std::array<std::pair<IMAGES, X>, N> const table_;

  NO_COPY(table);
  NO_MOVE_ASSIGN(table);
public:
  template <typename... T>
  explicit constexpr table(T... ts) : table_{ts...} {}

  MOVE_CONSTRUCTIBLE(table);

  constexpr X operator[](IMAGES const) = delete;
  constexpr X operator[](IMAGES const i) const
  {
    auto const index = static_cast<std::size_t>(i);
    return this->table_[index].second;
  }
};

struct resources
{
  resources() = delete;
  ~resources() = delete;

  static constexpr auto
  make_resource_table()
  {
    std::size_t constexpr N = 2;
    std::array<std::pair<IMAGES, table<N>::X>, N> constexpr TABLE{
      std::make_pair(IMAGES::CONTAINER, "assets/container.jpg"),
      std::make_pair(IMAGES::WALL, "assets/wall.jpg"),
    };
    return table<N>{TABLE};
  }
};

} // ns engine::gfx
