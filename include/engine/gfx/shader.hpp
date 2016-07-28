#pragma once
#include <stlw/result.hpp>
#include <engine/gfx/glew_gfx.hpp> // TODO: this file should be moved down into glew_gfx.

namespace engine
{
namespace gfx
{
  stlw::result<GLuint, std::string>
  load_shaders(char const* vertex_file_path, char const* fragment_file_path);
} // ns gfx
} // ns engine
