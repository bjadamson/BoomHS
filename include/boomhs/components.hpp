#pragma once
#include <glm/glm.hpp>
#include <opengl/colors.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>
#include <string>

namespace boomhs
{

struct Player
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

struct SkyboxRenderable
{
};

struct TextureRenderable
{
  opengl::TextureInfo texture_info;
};

} // ns boomhs
