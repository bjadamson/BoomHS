#pragma once
#include <glm/glm.hpp>
#include <stlw/type_macros.hpp>
#include <type_traits>

namespace gfx
{

template<typename B>
class pipeline
{
  B &backend_;

  explicit pipeline(B &b)
    : backend_(b)
  {
  }

  friend class pipeline_factory;
public:
  MOVE_CONSTRUCTIBLE_ONLY(pipeline);

  B& backend() { return this->backend_; }

  using CTX = typename B::CTX;
};

#define DEFINE_SHADER_FILENAME_TYPE(NAME)                                                          \
  struct NAME##_shader_filename {                                                                  \
    char const *filename;                                                                          \
    NAME##_shader_filename(char const *f)                                                          \
        : filename(f)                                                                              \
    {                                                                                              \
    }                                                                                              \
  }

DEFINE_SHADER_FILENAME_TYPE(vertex);
DEFINE_SHADER_FILENAME_TYPE(fragment);

#undef DEFINE_SHADER_FILENAME_TYPE

struct pipeline_factory
{
  pipeline_factory() = default;
  MOVE_CONSTRUCTIBLE_ONLY(pipeline_factory);

  template<typename B>
  auto
  make_pipeline(B &backend) const
  {
    return pipeline<B>{backend};
  }
};

} // ns gfx
