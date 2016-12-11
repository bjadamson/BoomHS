#pragma once
#include <stlw/type_macros.hpp>

namespace engine::gfx
{

enum class IMAGES { CONTAINER = 0, WALL, MULTICOLOR_RECT };

template <typename K, typename V, std::size_t N>
class table
{
private:
  std::array<std::pair<K, V>, N> const table_;

  NO_COPY(table);
  NO_MOVE_ASSIGN(table);

public:
  template <typename... T>
  explicit constexpr table(T... ts)
      : table_{ts...}
  {
  }

  MOVE_CONSTRUCTIBLE(table);

  constexpr V operator[](IMAGES const) = delete;
  constexpr V operator[](IMAGES const i) const
  {
    auto const index = static_cast<std::size_t>(i);
    return this->table_[index].second;
  }
};

struct resources {
  resources() = delete;
  ~resources() = delete;

  static constexpr auto make_resource_table()
  {
    using K = IMAGES;
    using V = char const *;
    std::size_t constexpr N = 3;

    return table<K, V, N>{
        std::make_pair(K::CONTAINER, "assets/container.jpg"),
        std::make_pair(K::WALL, "assets/wall.jpg"),
        std::make_pair(K::MULTICOLOR_RECT, "assets/multicolor_rect.jpg"),
    };
  }
};

} // ns engine::gfx
