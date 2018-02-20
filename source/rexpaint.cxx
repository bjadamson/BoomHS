#include <boomhs/rexpaint.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/types.hpp>
#include <zlib.h>

using namespace rexpaint;
using RexIOResult = stlw::result<stlw::empty_type, RexError>;

namespace
{

RexIOResult
make_rexerror(gzFile g)
{
  int errnum = 0;
  std::string const msg = gzerror(g, &errnum);
  return ::nonstd::unexpected_type<RexError>(RexError{errnum, msg});
}

RexIOResult
s_gzread(gzFile g, void *buf, unsigned int len)
{
  if (gzread(g, buf, len) > 0) {
    return stlw::empty_type{};
  }
  return make_rexerror(g);
}

RexIOResult
s_gzwrite(gzFile g, void *buf, unsigned int len)
{
  if (gzwrite(g, buf, len) > 0) {
    return stlw::empty_type{};
  }
  return make_rexerror(g);
}

stlw::result<gzFile, RexError>
s_gzopen(std::string const& filename, const char* permissions)
{
  gzFile g = gzopen(filename.c_str(), permissions);

  if (g != Z_NULL) {
    return g;
  }

  int errcode = 0;
  const char* errstr = gzerror(g, &errcode);
  if (errcode == 0) {
    std::string const msg{"gzerror. Assuming file '" + filename + "' does not exist."};
    RexError re{ERR_FILE_DOES_NOT_EXIST, msg};
    return stlw::create_error(MOVE(re));
  }
  return stlw::create_error(RexError{errcode, errstr});
}

} // ns anon

namespace rexpaint
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// RexTile
bool
RexTile::is_transparent() const
{
  //This might be faster than comparing with transparentTile(), despite it being a constexpr
  return (back_red == 255 && back_green == 0 && back_blue == 255);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RexLayer
RexLayer::RexLayer(Width const width, Height const height)
{
  tiles.resize(width * height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RexImage
RexImage::RexImage(Version const v, Width const w, Height const h, std::vector<RexLayer> && layers)
  : version_(v)
  , width_(w)
  , height_(h)
  , layers_(MOVE(layers))
{
  layers_.resize(num_layers());
  FORI(i, num_layers()) {
    layers_[i] = RexLayer{width(), height()};
  }

  // All layers above the first are set transparent.
  for (int l = 1; l < num_layers(); ++l) {
    FORI(i, width() * height()) {
      set_tile(l, i, TRANSPARENT_TILE);
    }
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
  FORI (i, width() * height()) {
    RexTile &overlay = get_tile(num_layers() - 1, i);
    if (!overlay.is_transparent()) {
      auto const layer = num_layers() - 2;
      get_tile(layer, i) = overlay;
    }
  }

  // Remove the last layer
  layers_.pop_back();

  // Recurse
  flatten();
}

stlw::result<RexImage, RexError>
RexImage::load(std::string const& filename)
{
  DO_TRY(auto gz, s_gzopen(filename.c_str(), "rb"));
  ON_SCOPE_EXIT([&]() { gzclose(gz); });

  int version;
  DO_EFFECT(s_gzread(gz, &version, sizeof(version)));

  int num_layers;
  std::vector<RexLayer> layers;
  DO_EFFECT(s_gzread(gz, &num_layers, sizeof(num_layers)));
  layers.resize(num_layers);

  int width, height;
  for (auto& layer : layers) {
    // The layer and height information is repeated.
    DO_EFFECT(s_gzread(gz, &width, sizeof(width)));
    DO_EFFECT(s_gzread(gz, &height, sizeof(height)));

    // Read the layer tiles
    layer = RexLayer{width, height};
    auto const num_bytes = sizeof(RexTile) * width * height;
    DO_EFFECT(s_gzread(gz, layer.tiles.data(), num_bytes));
  }

  return RexImage{version, width, height, MOVE(layers)};
}

stlw::result<stlw::empty_type, RexError>
RexImage::save(RexImage const& image, std::string const& filename)
{
  DO_TRY(gzFile gz, s_gzopen(filename.c_str(), "wb"));
  ON_SCOPE_EXIT([&]() { gzclose(gz); });

  auto const cast = [](auto *v) -> void*
  {
    return (void*)(v);
  };

  auto &version = image.version_;
  DO_EFFECT(s_gzwrite(gz, cast(&version), sizeof(version)));

  int const num_layers = image.layers_.size();
  DO_EFFECT(s_gzwrite(gz, cast(&num_layers), sizeof(num_layers)));

  int width, height;
  for (auto& layer : image.layers_) {
    // The layer and height information is repeated.
    s_gzwrite(gz, &width, sizeof(width));
    s_gzwrite(gz, &height, sizeof(height));

    // Write the layer tiles
    auto const num_bytes = sizeof(RexTile) * width * height;
    DO_EFFECT(s_gzwrite(gz, cast(layer.tiles.data()), num_bytes));
  }

  int const result = gzflush(gz, Z_FULL_FLUSH);
  if (Z_OK == result) {
    return make_rexerror(gz);
  }
  return stlw::empty_type();
}

} // ns rexpaint
