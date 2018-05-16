#pragma once
#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>
#include <opengl/bind.hpp>

#include <extlibs/glew.hpp>
#include <string>

namespace opengl
{

struct RBInfo
{
  DebugBoundCheck debug_check;
  GLuint          id;

  RBInfo();
  NO_COPY(RBInfo);
  MOVE_DEFAULT(RBInfo);

  // methods
  void bind_impl(stlw::Logger&);
  void unbind_impl(stlw::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();
  void destroy_impl();

  std::string to_string() const;

  static size_t constexpr NUM_BUFFERS = 1;
};
using RenderBuffer = stlw::AutoResource<RBInfo>;

} // ns opengl
