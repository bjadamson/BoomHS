#pragma once
#include <opengl/texture.hpp>
#include <string>

namespace boomhs
{

struct Player
{
};

struct Light
{
};

struct ShaderName
{
  std::string value;
};

struct CubeRenderable
{
};

struct MeshRenderable
{
  std::string name;
};

struct TextureRenderable
{
  opengl::TextureInfo texture_info;
};

} // ns boomhs
