#pragma once
#include <stlw/type_macros.hpp>

namespace engine::gfx
{

enum class IMAGES {
  CONTAINER = 0,
  WALL,
  MULTICOLOR_RECT,
  CUBE_FRONT,
  CUBE_RIGHT,
  CUBE_BACK,
  CUBE_LEFT,
  CUBE_TOP,
  CUBE_BOTTOM,
};

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
    std::size_t constexpr N = 9;

    return table<K, V, N>{
        std::make_pair(K::CONTAINER, "assets/container.jpg"),
        std::make_pair(K::WALL, "assets/wall.jpg"),
        std::make_pair(K::MULTICOLOR_RECT, "assets/multicolor_rect.jpg"),
        std::make_pair(K::CUBE_FRONT, "assets/cube_front.jpg"),
        std::make_pair(K::CUBE_RIGHT, "assets/cube_right.jpg"),
        std::make_pair(K::CUBE_BACK, "assets/cube_back.jpg"),
        std::make_pair(K::CUBE_LEFT, "assets/cube_left.jpg"),
        std::make_pair(K::CUBE_TOP, "assets/cube_top.jpg"),
        std::make_pair(K::CUBE_BOTTOM, "assets/cube_bottom.jpg"),
    };
  }
};

} // ns engine::gfx
