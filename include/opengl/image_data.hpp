#pragma once
#include <memory>
#include <SOIL.h>
#include <stlw/tuple.hpp>

namespace opengl
{

using pimage_t = std::unique_ptr<unsigned char, void (*)(unsigned char*)>;

struct image_data_t
{
  int width, height;
  pimage_t data;
};

template<typename L>
auto
load_image(L &logger, char const* path)
{
  int w = 0, h = 0;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, SOIL_LOAD_RGB);
  if (nullptr == pimage) {
    auto const fmt =
        fmt::sprintf("image at path '%s' failed to load, reason '%s'", path, SOIL_last_result());
    LOG_ERROR(fmt);
    std::abort();
  }
  pimage_t image_data{pimage, &SOIL_free_image_data};
  return image_data_t{w, h, MOVE(image_data)};
}

template<typename L, typename ...Paths>
auto
load_image(L &logger, Paths const&... paths)
{
  auto const load_fn = [&logger](auto const& path) {
    return load_image(logger, path);
  };
  auto const tuple = std::make_tuple(paths...);
  return stlw::map_tuple_elements(tuple, load_fn);
}

} // ns opengl
