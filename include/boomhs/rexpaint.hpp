#pragma once
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/fmt.hpp>
#include <stdint.h>
#include <array>
#include <ostream>
#include <string>
#include <vector>

// For version 1.03 of REXPaint
//
// Based on library from:
// https://github.com/pyridine/REXSpeeder
namespace rexpaint
{

// This struct matches the order and width of data in .xp tiles.
// Assumes little-endian format.
struct RexTile
{
  uint8_t character;

  // Do not use. Accounts for the unused final 3 bytes in .xp tile characters (v 1.03).
  std::array<uint8_t, 3> __padding;
  uint8_t fore_red;
  uint8_t fore_green;
  uint8_t fore_blue;
  uint8_t back_red;
  uint8_t back_green;
  uint8_t back_blue;

  // REXpaint identifies transparent tiles by setting their background color to 255,0,255.
  // You may want to check this for each tile before drawing or converting a RexFile.
  // (By default, no tile in the first layer is transaprent).
  bool is_transparent() const;
};

static constexpr int ERR_FILE_DOES_NOT_EXIST = 20202;
static constexpr auto REXPAINT_VERSION = 1.03;
static constexpr RexTile TRANSPARENT_TILE{0, {0, 0, 0}, 0, 0, 0, 255, 0, 255};

using Height = int;
using Width = int;
using LayerNumber = int;
using Version = int;

struct RexLayer
{
  std::vector<RexTile> tiles;
  RexLayer(Width, Height);
  RexLayer() = default;

  MOVE_ONLY(RexLayer);
};

struct RexError
{
  int const code;
  std::string const message;

  MOVE_CONSTRUCTIBLE_ONLY(RexError);
  RexError(int const c, std::string const& msg)
    : code(c)
    , message(msg)
  {
  }

  std::string
  to_string() const
  {
    return fmt::format("{code: '%i', msg: '%s'}", code, message);
  }
};

inline std::ostream&
operator<<(std::ostream &stream, RexError const& error)
{
  stream << error.to_string();
  return stream;
}

class RexImage
{
  Version version_;
  Width width_;
  Height height_;
  std::vector<RexLayer> layers_;

  RexImage(Version, Width, Height, std::vector<RexLayer> &&);
public:
  MOVE_CONSTRUCTIBLE_ONLY(RexImage);

  static Result<RexImage, RexError>
  load(std::string const&);

  // Save this RexFile into a valid .xp file that RexPaint can load (if the ".xp" suffix is present).
  static Result<stlw::empty_type, RexError>
  save(RexImage const&, std::string const&);

  // Image attributes
  auto version() const { return version_; };
  auto width() const { return width_; };
  auto height() const { return height_; };
  LayerNumber num_layers() const { return layers_.size(); };

  //Returns a pointer to a single tile specified by layer, x coordinate, y coordinate.
  //0,0 is the top-left corner.
  RexTile& get_tile(LayerNumber const layer, int const x, int const y);// { return &layers.at(layer).tiles.at(y + (x * height)); };

  //Returns a pointer to a single tile specified by layer and the actual index into the array.
  //Useful for iterating through a whole layer in one go for coordinate-nonspecific tasks.
  RexTile& get_tile(LayerNumber const layer, int const index);// { return &layers.at(layer).tiles.at(index); };

  void set_tile(LayerNumber layer, int x, int y, RexTile const&);
  void set_tile(LayerNumber layer, int i, RexTile const&);

  // Combines all the layers of the image into one layer.
  // Respects transparency.
  void flatten();
};

} // ns rexpaint
