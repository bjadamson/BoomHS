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

// Highly specialized macro, containing the shared body for ColorRGB/ColorRGBA types.
#define COLOR_RGB_IMPL                                                                             \
  float constexpr r() const { return this->colors_[0]; }                                           \
  float constexpr g() const { return this->colors_[1]; }                                           \
  float constexpr b() const { return this->colors_[2]; }                                           \
                                                                                                   \
  void set_r(float const v) { this->colors_[0] = v; }                                              \
  void set_g(float const v) { this->colors_[1] = v; }                                              \
  void set_b(float const v) { this->colors_[2] = v; }                                              \
                                                                                                   \
  void set_rgb(float const r, float const g, float const b)                                        \
  {                                                                                                \
    set_r(r);                                                                                      \
    set_g(g);                                                                                      \
    set_b(b);                                                                                      \
  }                                                                                                \
  auto const* data() const { return colors_.data(); }                                              \
  auto* data() { return colors_.data(); }                                                          \
                                                                                                   \
  auto constexpr vec3() const { return glm::vec3{r(), g(), b()}; }

class ColorRGB
{
  Color3 colors_ = {0, 0, 0};
public:
  ColorRGB() = default;

  explicit constexpr ColorRGB(Color3 const& c)
      : colors_(c)
  {
  }

  // non-explicit to allow construction from an initializer list.
  constexpr ColorRGB(float const r, float const g, float const b)
      : ColorRGB(common::make_array<float>(r, g, b))
  {
  }

  explicit constexpr ColorRGB(glm::vec3 const& v)
      : ColorRGB(v.x, v.y, v.z)
  {
  }

  COLOR_RGB_IMPL
  std::string to_string() const { return glm::to_string(vec3()); }
};

class ColorRGBA
{
  Color4 colors_ = {0, 0, 0, 0};

public:
  ColorRGBA() = default;

  explicit constexpr ColorRGBA(float const r, float const g, float const b, float const a)
      : colors_(common::make_array<float>(r, g, b, a))
  {
  }

  // non-explicit to allow construction from an initializer list.
  constexpr ColorRGBA(float const r, float const g, float const b)
      : ColorRGBA(r, g, b, DEFAULT_ALPHA)
  {
  }

  COLOR_RGB_IMPL
  float constexpr a() const { return this->colors_[3]; }
  void set_a(float const v) { this->colors_[3] = v; }

  void set_rgba(float const r, float const g, float const b, float const a)
  {
    set_rgb(r, g, b);
    set_a(a);
  }
  void set_rgba(ColorRGBA const& c) { set_rgba(c.r(), c.g(), c.b(), c.a()); }
  void set(ColorRGBA const& c) { set_rgba(c); }
  auto constexpr vec4() const
  {
    return glm::vec4{r(), g(), b(), a()};
  }

  auto constexpr rgb() const { return ColorRGB{vec3()}; }
  std::string to_string() const { return glm::to_string(vec4()); }
};
#undef COLOR_RGB_IMPL

inline auto
make_rgb(float const r, float const g, float const b)
{
  return ColorRGB{r, g, b};
}

inline auto
make_rgb(Color3 const& c)
{
  return ColorRGB{c[0], c[1], c[2]};
}

inline auto
make_rgb(glm::vec3 const& c)
{
  return ColorRGB{c.x, c.y, c.z};
}

////////////////////////////////////////////////////////////////////////
auto constexpr
make_rgba(float const r, float const g, float const b, float const a)
{
  return ColorRGBA{r, g, b, a};
}

auto constexpr
make_rgba(float const r, float const g, float const b)
{
  return make_rgba(r, g, b, DEFAULT_ALPHA);
}

auto constexpr
make_rgba(Color4 const& c)
{
  return make_rgba(c[0], c[1], c[2], c[3]);
}

auto constexpr
make_rgba(Color3 const& c)
{
  return make_rgba(c[0], c[1], c[2]);
}

auto constexpr
make_rgba(glm::vec3 const& vec)
{
  return make_rgba(vec.x, vec.y, vec.z);
}

auto constexpr
make_rgba(glm::vec4 const& vec)
{
  return make_rgba(vec.x, vec.y, vec.z, vec.w);
}

inline auto
random(RNG& rng, float const alpha = DEFAULT_ALPHA)
{
  auto const gen = [&rng]() { return rng.gen_0to1(); };
  return make_rgba(gen(), gen(), gen(), alpha);
}

inline auto
random(float const alpha = DEFAULT_ALPHA)
{
  boomhs::RNG rng;
  return random(rng);
}

inline auto
lerp(ColorRGB const& a, ColorRGB const& b, float const dt)
{
  return make_rgb(glm::lerp(a.vec3(), b.vec3(), dt));
}

inline auto
lerp(ColorRGBA const& a, ColorRGBA const& b, float const dt)
{
  return make_rgba(glm::lerp(a.vec4(), b.vec4(), dt));
}

inline std::ostream&
operator<<(std::ostream& os, ColorRGBA const& c)
{
  os << c.to_string();
  return os;
}

inline bool
operator==(ColorRGBA const& a, ColorRGBA const& b)
{
  return common::and_all(
      a.r() == b.r(),
      a.g() == b.g(),
      a.b() == b.b(),
      a.a() == b.a());
}

inline bool
operator!=(ColorRGBA const& a, ColorRGBA const& b)
{
  return !(a == b);
}

#define COLOR_LIST_MACRO(COLOR_T)                                                                \
  constexpr COLOR_T INDIAN_RED             = {0.804, 0.361, 0.361};                              \
  constexpr COLOR_T LIGHT_CORAL            = {0.941, 0.502, 0.502};                              \
  constexpr COLOR_T SALMON                 = {0.980, 0.502, 0.447};                              \
  constexpr COLOR_T DARK_SALMON            = {0.914, 0.588, 0.478};                              \
  constexpr COLOR_T LIGHT_SALMON           = {1.000, 0.627, 0.478};                              \
  constexpr COLOR_T CRIMSON                = {0.863, 0.078, 0.235};                              \
  constexpr COLOR_T RED                    = {1.000, 0.000, 0.000};                              \
  constexpr COLOR_T FIREBRICK              = {0.698, 0.133, 0.133};                              \
  constexpr COLOR_T DARKRED                = {0.545, 0.000, 0.000};                              \
  constexpr COLOR_T PINK                   = {1.000, 0.753, 0.796};                              \
  constexpr COLOR_T LIGHT_PINK             = {1.000, 0.714, 0.757};                              \
  constexpr COLOR_T HOT_PINK               = {1.000, 0.412, 0.706};                              \
  constexpr COLOR_T DEEP_PINK              = {1.000, 0.078, 0.576};                              \
  constexpr COLOR_T MEDIUM_VIOLET_RED      = {0.780, 0.082, 0.522};                              \
  constexpr COLOR_T PALE_VIOLET_RED        = {0.859, 0.439, 0.576};                              \
  constexpr COLOR_T CORAL                  = {1.000, 0.498, 0.314};                              \
  constexpr COLOR_T TOMATO                 = {1.000, 0.388, 0.278};                              \
  constexpr COLOR_T ORANGE_RED             = {1.000, 0.271, 0.000};                              \
  constexpr COLOR_T DARK_ORANGE            = {1.000, 0.549, 0.000};                              \
  constexpr COLOR_T ORANGE                 = {1.000, 0.647, 0.000};                              \
  constexpr COLOR_T GOLD                   = {1.000, 0.843, 0.000};                              \
  constexpr COLOR_T YELLOW                 = {1.000, 1.000, 0.000};                              \
  constexpr COLOR_T LIGHT_YELLOW           = {1.000, 1.000, 0.878};                              \
  constexpr COLOR_T LEMON_CHION            = {1.000, 0.980, 0.804};                              \
  constexpr COLOR_T LIGHT_GOLDENROD_YELLOW = {0.980, 0.980, 0.824};                              \
  constexpr COLOR_T PAPAYAWHIP             = {1.000, 0.937, 0.835};                              \
  constexpr COLOR_T MOCCASIN               = {1.000, 0.894, 0.710};                              \
  constexpr COLOR_T PEACHPU                = {1.000, 0.855, 0.725};                              \
  constexpr COLOR_T PALE_GOLDEN_ROD        = {0.933, 0.910, 0.667};                              \
  constexpr COLOR_T KHAKI                  = {0.941, 0.902, 0.549};                              \
  constexpr COLOR_T DARK_KHAKI             = {0.741, 0.718, 0.420};                              \
  constexpr COLOR_T LAVENDER               = {0.902, 0.902, 0.980};                              \
  constexpr COLOR_T THISTLE                = {0.847, 0.749, 0.847};                              \
  constexpr COLOR_T PLUM                   = {0.867, 0.627, 0.867};                              \
  constexpr COLOR_T VIOLET                 = {0.933, 0.510, 0.933};                              \
  constexpr COLOR_T ORCHID                 = {0.855, 0.439, 0.839};                              \
  constexpr COLOR_T FUCHSIA                = {1.000, 0.000, 1.000};                              \
  constexpr COLOR_T MAGENTA                = {1.000, 0.000, 1.000};                              \
  constexpr COLOR_T MEDIUM_ORCHID          = {0.729, 0.333, 0.827};                              \
  constexpr COLOR_T MEDIUM_PURPLE          = {0.576, 0.439, 0.859};                              \
  constexpr COLOR_T BLUE_VIOLET            = {0.541, 0.169, 0.886};                              \
  constexpr COLOR_T DARK_VIOLET            = {0.580, 0.000, 0.827};                              \
  constexpr COLOR_T DARK_ORCHID            = {0.600, 0.196, 0.800};                              \
  constexpr COLOR_T DARK_MAGENTA           = {0.545, 0.000, 0.545};                              \
  constexpr COLOR_T PURPLE                 = {0.502, 0.000, 0.502};                              \
  constexpr COLOR_T INDIGO                 = {0.294, 0.000, 0.510};                              \
  constexpr COLOR_T SLATE_BLUE             = {0.416, 0.353, 0.804};                              \
  constexpr COLOR_T DARK_SLATE_BLUE        = {0.282, 0.239, 0.545};                              \
  constexpr COLOR_T GREEN_YELLOW           = {0.678, 1.000, 0.184};                              \
  constexpr COLOR_T CHARTREUSE             = {0.498, 1.000, 0.000};                              \
  constexpr COLOR_T LAWN_GREEN             = {0.486, 0.988, 0.000};                              \
  constexpr COLOR_T LIME                   = {0.000, 1.000, 0.000};                              \
  constexpr COLOR_T LIME_GREEN             = {0.196, 0.804, 0.196};                              \
  constexpr COLOR_T PALE_GREEN             = {0.596, 0.984, 0.596};                              \
  constexpr COLOR_T LIGHT_GREEN            = {0.565, 0.933, 0.565};                              \
  constexpr COLOR_T MEDIUM_SPRING_GREEN    = {0.000, 0.980, 0.604};                              \
  constexpr COLOR_T SPRING_GREEN           = {0.000, 1.000, 0.498};                              \
  constexpr COLOR_T MEDIUM_SEA_GREEN       = {0.235, 0.702, 0.443};                              \
  constexpr COLOR_T SEA_GREEN              = {0.180, 0.545, 0.341};                              \
  constexpr COLOR_T FOREST_GREEN           = {0.133, 0.545, 0.133};                              \
  constexpr COLOR_T GREEN                  = {0.000, 0.502, 0.000};                              \
  constexpr COLOR_T DARKGREEN              = {0.000, 0.392, 0.000};                              \
  constexpr COLOR_T YELLOW_GREEN           = {0.604, 0.804, 0.196};                              \
  constexpr COLOR_T OLIVE_DRAB             = {0.420, 0.557, 0.137};                              \
  constexpr COLOR_T OLIVE                  = {0.502, 0.502, 0.000};                              \
  constexpr COLOR_T DARK_OLIVE_GREEN       = {0.333, 0.420, 0.184};                              \
  constexpr COLOR_T MEDIUM_AQUAMARINE      = {0.400, 0.804, 0.667};                              \
  constexpr COLOR_T DARK_SEAGREEN          = {0.561, 0.737, 0.561};                              \
  constexpr COLOR_T LIGHT_SEAGREEN         = {0.125, 0.698, 0.667};                              \
  constexpr COLOR_T DARK_CYAN              = {0.000, 0.545, 0.545};                              \
  constexpr COLOR_T TEAL                   = {0.000, 0.502, 0.502};                              \
  constexpr COLOR_T AQUA                   = {0.000, 1.000, 1.000};                              \
  constexpr COLOR_T CYAN                   = {0.000, 1.000, 1.000};                              \
  constexpr COLOR_T LIGHT_CYAN             = {0.878, 1.000, 1.000};                              \
  constexpr COLOR_T PALETURQUOISE          = {0.686, 0.933, 0.933};                              \
  constexpr COLOR_T AQUAMARINE             = {0.498, 1.000, 0.831};                              \
  constexpr COLOR_T TURQUOISE              = {0.251, 0.878, 0.816};                              \
  constexpr COLOR_T MEDIUM_TURQUOISE       = {0.282, 0.820, 0.800};                              \
  constexpr COLOR_T DARK_TURQUOISE         = {0.000, 0.808, 0.820};                              \
  constexpr COLOR_T CADET_BLUE             = {0.373, 0.620, 0.627};                              \
  constexpr COLOR_T STEEL_BLUE             = {0.275, 0.510, 0.706};                              \
  constexpr COLOR_T LIGHT_STEEL_BLUE       = {0.690, 0.769, 0.871};                              \
  constexpr COLOR_T POWDER_BLUE            = {0.690, 0.878, 0.902};                              \
  constexpr COLOR_T LIGHT_BLUE             = {0.678, 0.847, 0.902};                              \
  constexpr COLOR_T SKY_BLUE               = {0.529, 0.808, 0.922};                              \
  constexpr COLOR_T LIGHT_SKY_BLUE         = {0.529, 0.808, 0.980};                              \
  constexpr COLOR_T DEEP_SKY_BLUE          = {0.000, 0.749, 1.000};                              \
  constexpr COLOR_T DODGER_BLUE            = {0.118, 0.565, 1.000};                              \
  constexpr COLOR_T CORNLOWER_BLUE         = {0.392, 0.584, 0.929};                              \
  constexpr COLOR_T MEDIUM_SLATE_BLUE      = {0.482, 0.408, 0.933};                              \
  constexpr COLOR_T ROYAL_BLUE             = {0.255, 0.412, 0.882};                              \
  constexpr COLOR_T BLUE                   = {0.000, 0.000, 1.000};                              \
  constexpr COLOR_T MEDIUM_BLUE            = {0.000, 0.000, 0.804};                              \
  constexpr COLOR_T DARK_BLUE              = {0.000, 0.000, 0.545};                              \
  constexpr COLOR_T NAVY                   = {0.000, 0.000, 0.502};                              \
  constexpr COLOR_T MIDNIGHT_BLUE          = {0.098, 0.098, 0.439};                              \
  constexpr COLOR_T CORNSILK               = {1.000, 0.973, 0.863};                              \
  constexpr COLOR_T BLANCHED_ALMOND        = {1.000, 0.922, 0.804};                              \
  constexpr COLOR_T BISQUE                 = {1.000, 0.894, 0.769};                              \
  constexpr COLOR_T NAVAJ_OWHITE           = {1.000, 0.871, 0.678};                              \
  constexpr COLOR_T WHEAT                  = {0.961, 0.871, 0.702};                              \
  constexpr COLOR_T BURLY_WOOD             = {0.871, 0.722, 0.529};                              \
  constexpr COLOR_T TAN                    = {0.824, 0.706, 0.549};                              \
  constexpr COLOR_T ROSY_BROWN             = {0.737, 0.561, 0.561};                              \
  constexpr COLOR_T SANDY_BROWN            = {0.957, 0.643, 0.376};                              \
  constexpr COLOR_T GOLDENROD              = {0.855, 0.647, 0.125};                              \
  constexpr COLOR_T DARK_GOLDENROD         = {0.722, 0.525, 0.043};                              \
  constexpr COLOR_T PERU                   = {0.804, 0.522, 0.247};                              \
  constexpr COLOR_T CHOCOLATE              = {0.824, 0.412, 0.118};                              \
  constexpr COLOR_T SADDLE_BROWN           = {0.545, 0.271, 0.075};                              \
  constexpr COLOR_T SIENNA                 = {0.627, 0.322, 0.176};                              \
  constexpr COLOR_T BROWN                  = {0.647, 0.165, 0.165};                              \
  constexpr COLOR_T MAROON                 = {0.502, 0.000, 0.000};                              \
  constexpr COLOR_T WHITE                  = {1.000, 1.000, 1.000};                              \
  constexpr COLOR_T SNOW                   = {1.000, 0.980, 0.980};                              \
  constexpr COLOR_T HONEYDEW               = {0.941, 1.000, 0.941};                              \
  constexpr COLOR_T MINTCREAM              = {0.961, 1.000, 0.980};                              \
  constexpr COLOR_T AZURE                  = {0.941, 1.000, 1.000};                              \
  constexpr COLOR_T ALICEBLUE              = {0.941, 0.973, 1.000};                              \
  constexpr COLOR_T GHOST_WHITE            = {0.973, 0.973, 1.000};                              \
  constexpr COLOR_T WHITE_SMOKE            = {0.961, 0.961, 0.961};                              \
  constexpr COLOR_T SEASHELL               = {1.000, 0.961, 0.933};                              \
  constexpr COLOR_T BEIGE                  = {0.961, 0.961, 0.863};                              \
  constexpr COLOR_T OLDLACE                = {0.992, 0.961, 0.902};                              \
  constexpr COLOR_T FLORAL_WHITE           = {1.000, 0.980, 0.941};                              \
  constexpr COLOR_T IVORY                  = {1.000, 1.000, 0.941};                              \
  constexpr COLOR_T ANTIQUE_WHITE          = {0.980, 0.922, 0.843};                              \
  constexpr COLOR_T LINEN                  = {0.980, 0.941, 0.902};                              \
  constexpr COLOR_T LAVENDERBLUSH          = {1.000, 0.941, 0.961};                              \
  constexpr COLOR_T MISTYROSE              = {1.000, 0.894, 0.882};                              \
  constexpr COLOR_T GAINSBORO              = {0.863, 0.863, 0.863};                              \
  constexpr COLOR_T LIGHT_GREY             = {0.827, 0.827, 0.827};                              \
  constexpr COLOR_T SILVER                 = {0.753, 0.753, 0.753};                              \
  constexpr COLOR_T DARKGRAY               = {0.663, 0.663, 0.663};                              \
  constexpr COLOR_T GRAY                   = {0.502, 0.502, 0.502};                              \
  constexpr COLOR_T DIM_GRAY               = {0.412, 0.412, 0.412};                              \
  constexpr COLOR_T LIGHT_SLATE_GRAY       = {0.467, 0.533, 0.600};                              \
  constexpr COLOR_T SLATE_GRAY             = {0.439, 0.502, 0.565};                              \
  constexpr COLOR_T DARK_SLATE_GRAY        = {0.184, 0.310, 0.310};                              \
  constexpr COLOR_T BLACK                  = {0.000, 0.000, 0.000};                              \
                                                                                                 \
  constexpr COLOR_T NO_ALPHA               = {0.000, 0.000, 0.000};

namespace LIST_OF_COLORS3
{
COLOR_LIST_MACRO(ColorRGB);
} // namespace LIST_OF_COLORS3

namespace LIST_OF_COLORS4
{
COLOR_LIST_MACRO(ColorRGBA);
} // namespace LIST_OF_COLORS4

} // namespace boomhs::color

namespace boomhs
{
using ColorRGB  = color::ColorRGB;
using ColorRGBA = color::ColorRGBA;
using Color     = ColorRGBA;

template <size_t N>
using ColorArray = std::array<Color, N>;

} // namespace boomhs

namespace LOC3 = ::boomhs::color::LIST_OF_COLORS3;
namespace LOC4 = ::boomhs::color::LIST_OF_COLORS4;
