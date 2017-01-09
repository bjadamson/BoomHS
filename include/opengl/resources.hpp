#pragma once
#include <stlw/type_macros.hpp>

namespace opengl
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

  SB_FRONT,
  SB_RIGHT,
  SB_BACK,
  SB_LEFT,
  SB_TOP,
  SB_BOTTOM,

  HOUSE_ROOF,
  HOUSE_BODY,
  HOUSE_WINDOW_FRONT,
  HOUSE_WINDOW_LEFT,
  HOUSE_WINDOW_RIGHT
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
    std::size_t constexpr N = 3 + 6 + 6 + 5;

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

        std::make_pair(K::SB_FRONT,  "assets/sb_front.jpg"),
        std::make_pair(K::SB_RIGHT,  "assets/sb_right.jpg"),
        std::make_pair(K::SB_BACK,   "assets/sb_back.jpg"),
        std::make_pair(K::SB_LEFT,   "assets/sb_left.jpg"),
        std::make_pair(K::SB_TOP,    "assets/sb_top.jpg"),
        std::make_pair(K::SB_BOTTOM, "assets/sb_bottom.jpg"),

        std::make_pair(K::HOUSE_ROOF,  "assets/house_roof.jpg"),
        std::make_pair(K::HOUSE_BODY,  "assets/house_body.jpg"),
        std::make_pair(K::HOUSE_WINDOW_FRONT,  "assets/house_window_front.jpg"),
        std::make_pair(K::HOUSE_WINDOW_LEFT,  "assets/house_window_left.jpg"),
        std::make_pair(K::HOUSE_WINDOW_RIGHT,  "assets/house_window_right.jpg")
    };
  }
};

} // ns opengl
