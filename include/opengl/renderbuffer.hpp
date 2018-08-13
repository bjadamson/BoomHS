#pragma once
#include <common/auto_resource.hpp>
#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>
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
  void bind_impl(common::Logger&);
  void unbind_impl(common::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();
  void destroy_impl();

  std::string to_string() const;

  static size_t constexpr NUM_BUFFERS = 1;
};
using RenderBuffer = common::AutoResource<RBInfo>;

} // namespace opengl
