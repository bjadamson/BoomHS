#include <boomhs/rexpaint.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/result.hpp>

#include <iostream>
#include <zlib.h>

using namespace rexpaint;
using RexIOResult = Result<stlw::none_t, RexError>;

namespace
{

RexIOResult
make_rexerror(gzFile g)
{
  int               errnum = 0;
  std::string const msg    = gzerror(g, &errnum);
  return Err(RexError{errnum, msg});
}

RexIOResult
s_gzread(gzFile g, void* buf, unsigned int len)
{
  if (gzread(g, buf, len) > 0) {
    return OK_NONE;
  }
  return make_rexerror(g);
}

RexIOResult
s_gzwrite(gzFile g, void const* buf, unsigned int len)
{
  if (gzwrite(g, buf, len) > 0) {
    return OK_NONE;
  }
  return make_rexerror(g);
}

Result<gzFile, RexError>
s_gzopen(std::string const& filename, const char* permissions)
{
  gzFile g = gzopen(filename.c_str(), permissions);

  if (g != Z_NULL) {
    return Ok(g);
  }

  int         errcode = 0;
  const char* errstr  = gzerror(g, &errcode);
  if (errcode == 0) {
    std::string const msg{"gzerror. Assuming file '" + filename + "' does not exist."};
    RexError          re{ERR_FILE_DOES_NOT_EXIST, msg};
    return Err(MOVE(re));
  }
  return Err(RexError{errcode, errstr});
}

} // namespace

namespace rexpaint
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// RexTile
bool
RexTile::is_transparent() const
{
  // This might be faster than comparing with transparentTile(), despite it being a constexpr
  // clang-format off
  return ALLOF(
      back_red == 255,
      back_green == 0,
      back_blue == 255);
  // clang-format on
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RexLayer
RexLayer::RexLayer(Width const width, Height const height) { tiles.resize(width * height); }

////////////////////////////////////////////////////////////////////////////////////////////////////
// RexImage
RexImage::RexImage(Version const v, Width const w, Height const h, std::vector<RexLayer>&& layers)
    : version_(v)
    , width_(w)
    , height_(h)
    , layers_(MOVE(layers))
{
  layers_.resize(num_layers());
  FORI(i, num_layers()) { layers_[i] = RexLayer{width(), height()}; }

  // All layers above the first are set transparent.
  for (int l = 1; l < num_layers(); ++l) {
    FORI(i, width() * height()) { set_tile(l, i, TRANSPARENT_TILE); }
  }
}

RexTile&
RexImage::get_tile(LayerNumber const layer, int const x, int const y)
{
  return layers_[layer].tiles[y + (x * height_)];
}

RexTile&
RexImage::get_tile(LayerNumber const layer, int const index)
{
  return layers_[layer].tiles[index];
}

void
RexImage::set_tile(LayerNumber layer, int x, int y, RexTile const& val)
{
  get_tile(layer, x, y) = val;
}

void
RexImage::set_tile(LayerNumber layer, int i, RexTile const& val)
{
  get_tile(layer, i) = val;
}

void
RexImage::flatten()
{
  if (num_layers() == 1) {
    return;
  }

  // Paint the last layer onto the second-to-last
  FORI(i, width() * height())
  {
    RexTile& overlay = get_tile(num_layers() - 1, i);
    if (!overlay.is_transparent()) {
      auto const layer   = num_layers() - 2;
      get_tile(layer, i) = overlay;
    }
  }

  // Remove the last layer
  layers_.pop_back();

  // Recurse
  flatten();
}

Result<RexImage, RexError>
RexImage::load(std::string const& filename)
{
  auto gz = TRY_MOVEOUT(s_gzopen(filename.c_str(), "rb"));
  ON_SCOPE_EXIT([&]() { gzclose(gz); });

  int version;
  DO_EFFECT(s_gzread(gz, &version, sizeof(version)));

  int                   num_layers;
  std::vector<RexLayer> layers;
  DO_EFFECT(s_gzread(gz, &num_layers, sizeof(num_layers)));
  layers.resize(num_layers);

  int width, height;
  for (auto& layer : layers) {
    // The layer and height information is repeated.
    DO_EFFECT(s_gzread(gz, &width, sizeof(width)));

    DO_EFFECT(s_gzread(gz, &height, sizeof(height)));

    // Read the layer tiles
    layer                = RexLayer{width, height};
    auto const num_bytes = sizeof(RexTile) * width * height;
    DO_EFFECT(s_gzread(gz, layer.tiles.data(), num_bytes));
  }

  return Ok(RexImage{version, width, height, MOVE(layers)});
}

Result<stlw::none_t, RexError>
RexImage::save(RexImage const& image, std::string const& filename)
{
  gzFile gz = TRY_MOVEOUT(s_gzopen(filename.c_str(), "wb"));
  ON_SCOPE_EXIT([&]() { gzclose(gz); });

  auto const cast = [](auto* v) -> void const* { return static_cast<void const*>(v); };

  auto& version = image.version_;
  DO_EFFECT(s_gzwrite(gz, cast(&version), sizeof(version)));

  int const num_layers = image.layers_.size();
  DO_EFFECT(s_gzwrite(gz, cast(&num_layers), sizeof(num_layers)));

  int const width  = image.width();
  int const height = image.height();
  for (auto& layer : image.layers_) {
    // The layer and height information is repeated.
    s_gzwrite(gz, &width, sizeof(width));
    s_gzwrite(gz, &height, sizeof(height));

    // Write the layer tiles
    auto const num_bytes = sizeof(RexTile) * width * height;
    DO_EFFECT(s_gzwrite(gz, cast(layer.tiles.data()), num_bytes));
  }

  int const result = gzflush(gz, Z_FULL_FLUSH);
  if (Z_OK != result) {
    return make_rexerror(gz);
  }
  return OK_NONE;
}

} // namespace rexpaint
