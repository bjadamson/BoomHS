#pragma once
#include <array>
#include <boomhs/math.hpp>
#include <boomhs/random.hpp>
#include <common/algorithm.hpp>
#include <ostream>

namespace boomhs
{

class Color
{
  static constexpr float DEFAULT_ALPHA = 1.0f;

  std::array<float, 4> colors_ = common::make_array<float>(1.0f, 1.0f, 1.0f, DEFAULT_ALPHA);

public:
  static Color random(boomhs::RNG& rng, float const alpha = DEFAULT_ALPHA)
  {
    auto const gen = [&rng]() { return rng.gen_0to1(); };
    return Color{gen(), gen(), gen(), alpha};
  }

  static Color random(float const alpha = DEFAULT_ALPHA)
  {
    boomhs::RNG rng;
    return random(rng);
  }

  static Color lerp(Color const& a, Color const& b, float const dt)
  {
    return Color{glm::lerp(a.rgba(), b.rgba(), dt)};
  }

  Color() = default;
  explicit constexpr Color(float const r, float const g, float const b, float const a)
      : colors_(common::make_array<float>(r, g, b, a))
  {
  }

  constexpr Color(float const r, float const g, float const b)
      : Color(r, g, b, DEFAULT_ALPHA)
  {
  }

  explicit constexpr Color(std::array<float, 4> const& array)
      : Color(array[0], array[1], array[2], array[3])
  {
  }

  explicit constexpr Color(std::array<float, 3> const& array)
      : Color(array[0], array[1], array[2], DEFAULT_ALPHA)
  {
  }

  explicit constexpr Color(glm::vec3 const& vec)
      : Color(vec[0], vec[1], vec[2], DEFAULT_ALPHA)
  {
  }

  explicit constexpr Color(glm::vec4 const& vec)
      : Color(vec[0], vec[1], vec[2], vec[3])
  {
  }

  float r() const { return this->colors_[0]; }
  float g() const { return this->colors_[1]; }
  float b() const { return this->colors_[2]; }
  float a() const { return this->colors_[3]; }

  void set_r(float const v) { this->colors_[0] = v; }
  void set_g(float const v) { this->colors_[1] = v; }
  void set_b(float const v) { this->colors_[2] = v; }
  void set_a(float const v) { this->colors_[3] = v; }

  void set(float const r, float const g, float const b, float const a)
  {
    set_r(r);
    set_g(g);
    set_b(b);
    set_a(a);
  }
  void set(Color const& c) { set(c.r(), c.g(), c.b(), c.a()); }

  auto const* data() const { return colors_.data(); }

  auto* data() { return colors_.data(); }

  auto      rgb() const { return glm::vec3{r(), g(), b()}; }
  glm::vec4 rgba() const { return glm::vec4{r(), g(), b(), a()}; }

  std::string to_string() const { return glm::to_string(rgba()); }
};

inline std::ostream&
operator<<(std::ostream& os, Color const& c)
{
  os << c.to_string();
  return os;
}

template <size_t N>
using ColorArray = std::array<Color, N>;

namespace LIST_OF_COLORS
{
// clang-format off
  constexpr Color INDIAN_RED             = {0.804, 0.361, 0.361};
  constexpr Color LIGHT_CORAL            = {0.941, 0.502, 0.502};
  constexpr Color SALMON                 = {0.980, 0.502, 0.447};
  constexpr Color DARK_SALMON            = {0.914, 0.588, 0.478};
  constexpr Color LIGHT_SALMON           = {1.000, 0.627, 0.478};
  constexpr Color CRIMSON                = {0.863, 0.078, 0.235};
  constexpr Color RED                    = {1.000, 0.000, 0.000};
  constexpr Color FIREBRICK              = {0.698, 0.133, 0.133};
  constexpr Color DARKRED                = {0.545, 0.000, 0.000};
  constexpr Color PINK                   = {1.000, 0.753, 0.796};
  constexpr Color LIGHT_PINK             = {1.000, 0.714, 0.757};
  constexpr Color HOT_PINK               = {1.000, 0.412, 0.706};
  constexpr Color DEEP_PINK              = {1.000, 0.078, 0.576};
  constexpr Color MEDIUM_VIOLET_RED      = {0.780, 0.082, 0.522};
  constexpr Color PALE_VIOLET_RED        = {0.859, 0.439, 0.576};
  constexpr Color CORAL                  = {1.000, 0.498, 0.314};
  constexpr Color TOMATO                 = {1.000, 0.388, 0.278};
  constexpr Color ORANGE_RED             = {1.000, 0.271, 0.000};
  constexpr Color DARK_ORANGE            = {1.000, 0.549, 0.000};
  constexpr Color ORANGE                 = {1.000, 0.647, 0.000};
  constexpr Color GOLD                   = {1.000, 0.843, 0.000};
  constexpr Color YELLOW                 = {1.000, 1.000, 0.000};
  constexpr Color LIGHT_YELLOW           = {1.000, 1.000, 0.878};
  constexpr Color LEMON_CHION            = {1.000, 0.980, 0.804};
  constexpr Color LIGHT_GOLDENROD_YELLOW = {0.980, 0.980, 0.824};
  constexpr Color PAPAYAWHIP             = {1.000, 0.937, 0.835};
  constexpr Color MOCCASIN               = {1.000, 0.894, 0.710};
  constexpr Color PEACHPU                = {1.000, 0.855, 0.725};
  constexpr Color PALE_GOLDEN_ROD        = {0.933, 0.910, 0.667};
  constexpr Color KHAKI                  = {0.941, 0.902, 0.549};
  constexpr Color DARK_KHAKI             = {0.741, 0.718, 0.420};
  constexpr Color LAVENDER               = {0.902, 0.902, 0.980};
  constexpr Color THISTLE                = {0.847, 0.749, 0.847};
  constexpr Color PLUM                   = {0.867, 0.627, 0.867};
  constexpr Color VIOLET                 = {0.933, 0.510, 0.933};
  constexpr Color ORCHID                 = {0.855, 0.439, 0.839};
  constexpr Color FUCHSIA                = {1.000, 0.000, 1.000};
  constexpr Color MAGENTA                = {1.000, 0.000, 1.000};
  constexpr Color MEDIUM_ORCHID          = {0.729, 0.333, 0.827};
  constexpr Color MEDIUM_PURPLE          = {0.576, 0.439, 0.859};
  constexpr Color BLUE_VIOLET            = {0.541, 0.169, 0.886};
  constexpr Color DARK_VIOLET            = {0.580, 0.000, 0.827};
  constexpr Color DARK_ORCHID            = {0.600, 0.196, 0.800};
  constexpr Color DARK_MAGENTA           = {0.545, 0.000, 0.545};
  constexpr Color PURPLE                 = {0.502, 0.000, 0.502};
  constexpr Color INDIGO                 = {0.294, 0.000, 0.510};
  constexpr Color SLATE_BLUE             = {0.416, 0.353, 0.804};
  constexpr Color DARK_SLATE_BLUE        = {0.282, 0.239, 0.545};
  constexpr Color GREEN_YELLOW           = {0.678, 1.000, 0.184};
  constexpr Color CHARTREUSE             = {0.498, 1.000, 0.000};
  constexpr Color LAWN_GREEN             = {0.486, 0.988, 0.000};
  constexpr Color LIME                   = {0.000, 1.000, 0.000};
  constexpr Color LIME_GREEN             = {0.196, 0.804, 0.196};
  constexpr Color PALE_GREEN             = {0.596, 0.984, 0.596};
  constexpr Color LIGHT_GREEN            = {0.565, 0.933, 0.565};
  constexpr Color MEDIUM_SPRING_GREEN    = {0.000, 0.980, 0.604};
  constexpr Color SPRING_GREEN           = {0.000, 1.000, 0.498};
  constexpr Color MEDIUM_SEA_GREEN       = {0.235, 0.702, 0.443};
  constexpr Color SEA_GREEN              = {0.180, 0.545, 0.341};
  constexpr Color FOREST_GREEN           = {0.133, 0.545, 0.133};
  constexpr Color GREEN                  = {0.000, 0.502, 0.000};
  constexpr Color DARKGREEN              = {0.000, 0.392, 0.000};
  constexpr Color YELLOW_GREEN           = {0.604, 0.804, 0.196};
  constexpr Color OLIVE_DRAB             = {0.420, 0.557, 0.137};
  constexpr Color OLIVE                  = {0.502, 0.502, 0.000};
  constexpr Color DARK_OLIVE_GREEN       = {0.333, 0.420, 0.184};
  constexpr Color MEDIUM_AQUAMARINE      = {0.400, 0.804, 0.667};
  constexpr Color DARK_SEAGREEN          = {0.561, 0.737, 0.561};
  constexpr Color LIGHT_SEAGREEN         = {0.125, 0.698, 0.667};
  constexpr Color DARK_CYAN              = {0.000, 0.545, 0.545};
  constexpr Color TEAL                   = {0.000, 0.502, 0.502};
  constexpr Color AQUA                   = {0.000, 1.000, 1.000};
  constexpr Color CYAN                   = {0.000, 1.000, 1.000};
  constexpr Color LIGHT_CYAN             = {0.878, 1.000, 1.000};
  constexpr Color PALETURQUOISE          = {0.686, 0.933, 0.933};
  constexpr Color AQUAMARINE             = {0.498, 1.000, 0.831};
  constexpr Color TURQUOISE              = {0.251, 0.878, 0.816};
  constexpr Color MEDIUM_TURQUOISE       = {0.282, 0.820, 0.800};
  constexpr Color DARK_TURQUOISE         = {0.000, 0.808, 0.820};
  constexpr Color CADET_BLUE             = {0.373, 0.620, 0.627};
  constexpr Color STEEL_BLUE             = {0.275, 0.510, 0.706};
  constexpr Color LIGHT_STEEL_BLUE       = {0.690, 0.769, 0.871};
  constexpr Color POWDER_BLUE            = {0.690, 0.878, 0.902};
  constexpr Color LIGHT_BLUE             = {0.678, 0.847, 0.902};
  constexpr Color SKY_BLUE               = {0.529, 0.808, 0.922};
  constexpr Color LIGHT_SKY_BLUE         = {0.529, 0.808, 0.980};
  constexpr Color DEEP_SKY_BLUE          = {0.000, 0.749, 1.000};
  constexpr Color DODGER_BLUE            = {0.118, 0.565, 1.000};
  constexpr Color CORNLOWER_BLUE         = {0.392, 0.584, 0.929};
  constexpr Color MEDIUM_SLATE_BLUE      = {0.482, 0.408, 0.933};
  constexpr Color ROYAL_BLUE             = {0.255, 0.412, 0.882};
  constexpr Color BLUE                   = {0.000, 0.000, 1.000};
  constexpr Color MEDIUM_BLUE            = {0.000, 0.000, 0.804};
  constexpr Color DARK_BLUE              = {0.000, 0.000, 0.545};
  constexpr Color NAVY                   = {0.000, 0.000, 0.502};
  constexpr Color MIDNIGHT_BLUE          = {0.098, 0.098, 0.439};
  constexpr Color CORNSILK               = {1.000, 0.973, 0.863};
  constexpr Color BLANCHED_ALMOND        = {1.000, 0.922, 0.804};
  constexpr Color BISQUE                 = {1.000, 0.894, 0.769};
  constexpr Color NAVAJ_OWHITE           = {1.000, 0.871, 0.678};
  constexpr Color WHEAT                  = {0.961, 0.871, 0.702};
  constexpr Color BURLY_WOOD             = {0.871, 0.722, 0.529};
  constexpr Color TAN                    = {0.824, 0.706, 0.549};
  constexpr Color ROSY_BROWN             = {0.737, 0.561, 0.561};
  constexpr Color SANDY_BROWN            = {0.957, 0.643, 0.376};
  constexpr Color GOLDENROD              = {0.855, 0.647, 0.125};
  constexpr Color DARK_GOLDENROD         = {0.722, 0.525, 0.043};
  constexpr Color PERU                   = {0.804, 0.522, 0.247};
  constexpr Color CHOCOLATE              = {0.824, 0.412, 0.118};
  constexpr Color SADDLE_BROWN           = {0.545, 0.271, 0.075};
  constexpr Color SIENNA                 = {0.627, 0.322, 0.176};
  constexpr Color BROWN                  = {0.647, 0.165, 0.165};
  constexpr Color MAROON                 = {0.502, 0.000, 0.000};
  constexpr Color WHITE                  = {1.000, 1.000, 1.000};
  constexpr Color SNOW                   = {1.000, 0.980, 0.980};
  constexpr Color HONEYDEW               = {0.941, 1.000, 0.941};
  constexpr Color MINTCREAM              = {0.961, 1.000, 0.980};
  constexpr Color AZURE                  = {0.941, 1.000, 1.000};
  constexpr Color ALICEBLUE              = {0.941, 0.973, 1.000};
  constexpr Color GHOST_WHITE            = {0.973, 0.973, 1.000};
  constexpr Color WHITE_SMOKE            = {0.961, 0.961, 0.961};
  constexpr Color SEASHELL               = {1.000, 0.961, 0.933};
  constexpr Color BEIGE                  = {0.961, 0.961, 0.863};
  constexpr Color OLDLACE                = {0.992, 0.961, 0.902};
  constexpr Color FLORAL_WHITE           = {1.000, 0.980, 0.941};
  constexpr Color IVORY                  = {1.000, 1.000, 0.941};
  constexpr Color ANTIQUE_WHITE          = {0.980, 0.922, 0.843};
  constexpr Color LINEN                  = {0.980, 0.941, 0.902};
  constexpr Color LAVENDERBLUSH          = {1.000, 0.941, 0.961};
  constexpr Color MISTYROSE              = {1.000, 0.894, 0.882};
  constexpr Color GAINSBORO              = {0.863, 0.863, 0.863};
  constexpr Color LIGHT_GREY             = {0.827, 0.827, 0.827};
  constexpr Color SILVER                 = {0.753, 0.753, 0.753};
  constexpr Color DARKGRAY               = {0.663, 0.663, 0.663};
  constexpr Color GRAY                   = {0.502, 0.502, 0.502};
  constexpr Color DIM_GRAY               = {0.412, 0.412, 0.412};
  constexpr Color LIGHT_SLATE_GRAY       = {0.467, 0.533, 0.600};
  constexpr Color SLATE_GRAY             = {0.439, 0.502, 0.565};
  constexpr Color DARK_SLATE_GRAY        = {0.184, 0.310, 0.310};
  constexpr Color BLACK                  = {0.000, 0.000, 0.000};
// clang-format on
} // namespace LIST_OF_COLORS
} // namespace boomhs

namespace LOC = boomhs::LIST_OF_COLORS;
