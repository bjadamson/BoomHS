#pragma once
#include <array>
#include <boomhs/math.hpp>
#include <boomhs/random.hpp>
#include <common/algorithm.hpp>
#include <ostream>

namespace boomhs::color
{

static constexpr float DEFAULT_ALPHA = 1.0f;
using Color3 = std::array<float, 3>;
using Color4 = std::array<float, 4>;

class ColorRGBA
{
  std::array<float, 4> colors_ = {0, 0, 0, 0};

public:
  ColorRGBA() = default;

  constexpr ColorRGBA(float const r, float const g, float const b)
      : ColorRGBA(r, g, b, DEFAULT_ALPHA)
  {
  }

  constexpr ColorRGBA(float const r, float const g, float const b, float const a)
      : colors_(common::make_array<float>(r, g, b, a))
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
  void set(ColorRGBA const& c) { set(c.r(), c.g(), c.b(), c.a()); }

  auto const* data() const { return colors_.data(); }
  auto* data() { return colors_.data(); }

  auto rgb()  const { return glm::vec3{r(), g(), b()}; }
  auto rgba() const { return glm::vec4{r(), g(), b(), a()}; }

  std::string to_string() const { return glm::to_string(rgba()); }
};

auto constexpr
make(float const r, float const g, float const b, float const a)
{
  return ColorRGBA{r, g, b, a};
}

auto constexpr
make(float const r, float const g, float const b)
{
  return make(r, g, b, DEFAULT_ALPHA);
}

auto constexpr
make(Color4 const& c)
{
  return make(c[0], c[1], c[2], c[3]);
}

auto constexpr
make(Color3 const& c)
{
  return make(c[0], c[1], c[2]);
}

auto constexpr
make(glm::vec3 const& vec)
{
  return make(vec.x, vec.y, vec.z);
}

auto constexpr
make(glm::vec4 const& vec)
{
  return make(vec.x, vec.y, vec.z, vec.w);
}

// static methods
inline auto
random(RNG& rng, float const alpha = DEFAULT_ALPHA)
{
  auto const gen = [&rng]() { return rng.gen_0to1(); };
  return ColorRGBA{gen(), gen(), gen(), alpha};
}

inline auto
random(float const alpha = DEFAULT_ALPHA)
{
  boomhs::RNG rng;
  return random(rng);
}

inline auto
lerp(ColorRGBA const& a, ColorRGBA const& b, float const dt)
{
  return make(glm::lerp(a.rgba(), b.rgba(), dt));
}

inline std::ostream&
operator<<(std::ostream& os, ColorRGBA const& c)
{
  os << c.to_string();
  return os;
}

namespace LIST_OF_COLORS
{
// clang-format off
  constexpr ColorRGBA INDIAN_RED             = {0.804, 0.361, 0.361};
  constexpr ColorRGBA LIGHT_CORAL            = {0.941, 0.502, 0.502};
  constexpr ColorRGBA SALMON                 = {0.980, 0.502, 0.447};
  constexpr ColorRGBA DARK_SALMON            = {0.914, 0.588, 0.478};
  constexpr ColorRGBA LIGHT_SALMON           = {1.000, 0.627, 0.478};
  constexpr ColorRGBA CRIMSON                = {0.863, 0.078, 0.235};
  constexpr ColorRGBA RED                    = {1.000, 0.000, 0.000};
  constexpr ColorRGBA FIREBRICK              = {0.698, 0.133, 0.133};
  constexpr ColorRGBA DARKRED                = {0.545, 0.000, 0.000};
  constexpr ColorRGBA PINK                   = {1.000, 0.753, 0.796};
  constexpr ColorRGBA LIGHT_PINK             = {1.000, 0.714, 0.757};
  constexpr ColorRGBA HOT_PINK               = {1.000, 0.412, 0.706};
  constexpr ColorRGBA DEEP_PINK              = {1.000, 0.078, 0.576};
  constexpr ColorRGBA MEDIUM_VIOLET_RED      = {0.780, 0.082, 0.522};
  constexpr ColorRGBA PALE_VIOLET_RED        = {0.859, 0.439, 0.576};
  constexpr ColorRGBA CORAL                  = {1.000, 0.498, 0.314};
  constexpr ColorRGBA TOMATO                 = {1.000, 0.388, 0.278};
  constexpr ColorRGBA ORANGE_RED             = {1.000, 0.271, 0.000};
  constexpr ColorRGBA DARK_ORANGE            = {1.000, 0.549, 0.000};
  constexpr ColorRGBA ORANGE                 = {1.000, 0.647, 0.000};
  constexpr ColorRGBA GOLD                   = {1.000, 0.843, 0.000};
  constexpr ColorRGBA YELLOW                 = {1.000, 1.000, 0.000};
  constexpr ColorRGBA LIGHT_YELLOW           = {1.000, 1.000, 0.878};
  constexpr ColorRGBA LEMON_CHION            = {1.000, 0.980, 0.804};
  constexpr ColorRGBA LIGHT_GOLDENROD_YELLOW = {0.980, 0.980, 0.824};
  constexpr ColorRGBA PAPAYAWHIP             = {1.000, 0.937, 0.835};
  constexpr ColorRGBA MOCCASIN               = {1.000, 0.894, 0.710};
  constexpr ColorRGBA PEACHPU                = {1.000, 0.855, 0.725};
  constexpr ColorRGBA PALE_GOLDEN_ROD        = {0.933, 0.910, 0.667};
  constexpr ColorRGBA KHAKI                  = {0.941, 0.902, 0.549};
  constexpr ColorRGBA DARK_KHAKI             = {0.741, 0.718, 0.420};
  constexpr ColorRGBA LAVENDER               = {0.902, 0.902, 0.980};
  constexpr ColorRGBA THISTLE                = {0.847, 0.749, 0.847};
  constexpr ColorRGBA PLUM                   = {0.867, 0.627, 0.867};
  constexpr ColorRGBA VIOLET                 = {0.933, 0.510, 0.933};
  constexpr ColorRGBA ORCHID                 = {0.855, 0.439, 0.839};
  constexpr ColorRGBA FUCHSIA                = {1.000, 0.000, 1.000};
  constexpr ColorRGBA MAGENTA                = {1.000, 0.000, 1.000};
  constexpr ColorRGBA MEDIUM_ORCHID          = {0.729, 0.333, 0.827};
  constexpr ColorRGBA MEDIUM_PURPLE          = {0.576, 0.439, 0.859};
  constexpr ColorRGBA BLUE_VIOLET            = {0.541, 0.169, 0.886};
  constexpr ColorRGBA DARK_VIOLET            = {0.580, 0.000, 0.827};
  constexpr ColorRGBA DARK_ORCHID            = {0.600, 0.196, 0.800};
  constexpr ColorRGBA DARK_MAGENTA           = {0.545, 0.000, 0.545};
  constexpr ColorRGBA PURPLE                 = {0.502, 0.000, 0.502};
  constexpr ColorRGBA INDIGO                 = {0.294, 0.000, 0.510};
  constexpr ColorRGBA SLATE_BLUE             = {0.416, 0.353, 0.804};
  constexpr ColorRGBA DARK_SLATE_BLUE        = {0.282, 0.239, 0.545};
  constexpr ColorRGBA GREEN_YELLOW           = {0.678, 1.000, 0.184};
  constexpr ColorRGBA CHARTREUSE             = {0.498, 1.000, 0.000};
  constexpr ColorRGBA LAWN_GREEN             = {0.486, 0.988, 0.000};
  constexpr ColorRGBA LIME                   = {0.000, 1.000, 0.000};
  constexpr ColorRGBA LIME_GREEN             = {0.196, 0.804, 0.196};
  constexpr ColorRGBA PALE_GREEN             = {0.596, 0.984, 0.596};
  constexpr ColorRGBA LIGHT_GREEN            = {0.565, 0.933, 0.565};
  constexpr ColorRGBA MEDIUM_SPRING_GREEN    = {0.000, 0.980, 0.604};
  constexpr ColorRGBA SPRING_GREEN           = {0.000, 1.000, 0.498};
  constexpr ColorRGBA MEDIUM_SEA_GREEN       = {0.235, 0.702, 0.443};
  constexpr ColorRGBA SEA_GREEN              = {0.180, 0.545, 0.341};
  constexpr ColorRGBA FOREST_GREEN           = {0.133, 0.545, 0.133};
  constexpr ColorRGBA GREEN                  = {0.000, 0.502, 0.000};
  constexpr ColorRGBA DARKGREEN              = {0.000, 0.392, 0.000};
  constexpr ColorRGBA YELLOW_GREEN           = {0.604, 0.804, 0.196};
  constexpr ColorRGBA OLIVE_DRAB             = {0.420, 0.557, 0.137};
  constexpr ColorRGBA OLIVE                  = {0.502, 0.502, 0.000};
  constexpr ColorRGBA DARK_OLIVE_GREEN       = {0.333, 0.420, 0.184};
  constexpr ColorRGBA MEDIUM_AQUAMARINE      = {0.400, 0.804, 0.667};
  constexpr ColorRGBA DARK_SEAGREEN          = {0.561, 0.737, 0.561};
  constexpr ColorRGBA LIGHT_SEAGREEN         = {0.125, 0.698, 0.667};
  constexpr ColorRGBA DARK_CYAN              = {0.000, 0.545, 0.545};
  constexpr ColorRGBA TEAL                   = {0.000, 0.502, 0.502};
  constexpr ColorRGBA AQUA                   = {0.000, 1.000, 1.000};
  constexpr ColorRGBA CYAN                   = {0.000, 1.000, 1.000};
  constexpr ColorRGBA LIGHT_CYAN             = {0.878, 1.000, 1.000};
  constexpr ColorRGBA PALETURQUOISE          = {0.686, 0.933, 0.933};
  constexpr ColorRGBA AQUAMARINE             = {0.498, 1.000, 0.831};
  constexpr ColorRGBA TURQUOISE              = {0.251, 0.878, 0.816};
  constexpr ColorRGBA MEDIUM_TURQUOISE       = {0.282, 0.820, 0.800};
  constexpr ColorRGBA DARK_TURQUOISE         = {0.000, 0.808, 0.820};
  constexpr ColorRGBA CADET_BLUE             = {0.373, 0.620, 0.627};
  constexpr ColorRGBA STEEL_BLUE             = {0.275, 0.510, 0.706};
  constexpr ColorRGBA LIGHT_STEEL_BLUE       = {0.690, 0.769, 0.871};
  constexpr ColorRGBA POWDER_BLUE            = {0.690, 0.878, 0.902};
  constexpr ColorRGBA LIGHT_BLUE             = {0.678, 0.847, 0.902};
  constexpr ColorRGBA SKY_BLUE               = {0.529, 0.808, 0.922};
  constexpr ColorRGBA LIGHT_SKY_BLUE         = {0.529, 0.808, 0.980};
  constexpr ColorRGBA DEEP_SKY_BLUE          = {0.000, 0.749, 1.000};
  constexpr ColorRGBA DODGER_BLUE            = {0.118, 0.565, 1.000};
  constexpr ColorRGBA CORNLOWER_BLUE         = {0.392, 0.584, 0.929};
  constexpr ColorRGBA MEDIUM_SLATE_BLUE      = {0.482, 0.408, 0.933};
  constexpr ColorRGBA ROYAL_BLUE             = {0.255, 0.412, 0.882};
  constexpr ColorRGBA BLUE                   = {0.000, 0.000, 1.000};
  constexpr ColorRGBA MEDIUM_BLUE            = {0.000, 0.000, 0.804};
  constexpr ColorRGBA DARK_BLUE              = {0.000, 0.000, 0.545};
  constexpr ColorRGBA NAVY                   = {0.000, 0.000, 0.502};
  constexpr ColorRGBA MIDNIGHT_BLUE          = {0.098, 0.098, 0.439};
  constexpr ColorRGBA CORNSILK               = {1.000, 0.973, 0.863};
  constexpr ColorRGBA BLANCHED_ALMOND        = {1.000, 0.922, 0.804};
  constexpr ColorRGBA BISQUE                 = {1.000, 0.894, 0.769};
  constexpr ColorRGBA NAVAJ_OWHITE           = {1.000, 0.871, 0.678};
  constexpr ColorRGBA WHEAT                  = {0.961, 0.871, 0.702};
  constexpr ColorRGBA BURLY_WOOD             = {0.871, 0.722, 0.529};
  constexpr ColorRGBA TAN                    = {0.824, 0.706, 0.549};
  constexpr ColorRGBA ROSY_BROWN             = {0.737, 0.561, 0.561};
  constexpr ColorRGBA SANDY_BROWN            = {0.957, 0.643, 0.376};
  constexpr ColorRGBA GOLDENROD              = {0.855, 0.647, 0.125};
  constexpr ColorRGBA DARK_GOLDENROD         = {0.722, 0.525, 0.043};
  constexpr ColorRGBA PERU                   = {0.804, 0.522, 0.247};
  constexpr ColorRGBA CHOCOLATE              = {0.824, 0.412, 0.118};
  constexpr ColorRGBA SADDLE_BROWN           = {0.545, 0.271, 0.075};
  constexpr ColorRGBA SIENNA                 = {0.627, 0.322, 0.176};
  constexpr ColorRGBA BROWN                  = {0.647, 0.165, 0.165};
  constexpr ColorRGBA MAROON                 = {0.502, 0.000, 0.000};
  constexpr ColorRGBA WHITE                  = {1.000, 1.000, 1.000};
  constexpr ColorRGBA SNOW                   = {1.000, 0.980, 0.980};
  constexpr ColorRGBA HONEYDEW               = {0.941, 1.000, 0.941};
  constexpr ColorRGBA MINTCREAM              = {0.961, 1.000, 0.980};
  constexpr ColorRGBA AZURE                  = {0.941, 1.000, 1.000};
  constexpr ColorRGBA ALICEBLUE              = {0.941, 0.973, 1.000};
  constexpr ColorRGBA GHOST_WHITE            = {0.973, 0.973, 1.000};
  constexpr ColorRGBA WHITE_SMOKE            = {0.961, 0.961, 0.961};
  constexpr ColorRGBA SEASHELL               = {1.000, 0.961, 0.933};
  constexpr ColorRGBA BEIGE                  = {0.961, 0.961, 0.863};
  constexpr ColorRGBA OLDLACE                = {0.992, 0.961, 0.902};
  constexpr ColorRGBA FLORAL_WHITE           = {1.000, 0.980, 0.941};
  constexpr ColorRGBA IVORY                  = {1.000, 1.000, 0.941};
  constexpr ColorRGBA ANTIQUE_WHITE          = {0.980, 0.922, 0.843};
  constexpr ColorRGBA LINEN                  = {0.980, 0.941, 0.902};
  constexpr ColorRGBA LAVENDERBLUSH          = {1.000, 0.941, 0.961};
  constexpr ColorRGBA MISTYROSE              = {1.000, 0.894, 0.882};
  constexpr ColorRGBA GAINSBORO              = {0.863, 0.863, 0.863};
  constexpr ColorRGBA LIGHT_GREY             = {0.827, 0.827, 0.827};
  constexpr ColorRGBA SILVER                 = {0.753, 0.753, 0.753};
  constexpr ColorRGBA DARKGRAY               = {0.663, 0.663, 0.663};
  constexpr ColorRGBA GRAY                   = {0.502, 0.502, 0.502};
  constexpr ColorRGBA DIM_GRAY               = {0.412, 0.412, 0.412};
  constexpr ColorRGBA LIGHT_SLATE_GRAY       = {0.467, 0.533, 0.600};
  constexpr ColorRGBA SLATE_GRAY             = {0.439, 0.502, 0.565};
  constexpr ColorRGBA DARK_SLATE_GRAY        = {0.184, 0.310, 0.310};
  constexpr ColorRGBA BLACK                  = {0.000, 0.000, 0.000};

  constexpr ColorRGBA NO_ALPHA               = {0.000, 0.000, 0.000, 0.000};
// clang-format on
} // namespace LIST_OF_COLORS
} // namespace boomhs::color

namespace boomhs
{
using Color   = color::ColorRGBA;
namespace LOC = color::LIST_OF_COLORS;

template <size_t N>
using ColorArray = std::array<Color, N>;

} // namespace boomhs

