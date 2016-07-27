#pragma once
#include <engine/gfx/glew_gfx.hpp> // TODO: weird to specify glew_gfx in two places.
#include <stlw/expected.hpp>

namespace engine
{
namespace gfx
{
  boost::expected<GLuint, std::string>
  LoadShaders(char const* vertex_file_path, char const* fragment_file_path);
} // ns gfx
} // ns engine
