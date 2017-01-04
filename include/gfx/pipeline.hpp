#pragma once
#include <glm/glm.hpp>
#include <stlw/type_macros.hpp>

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

  B const& backend() const { return this->backend_; }

  /*
  void use()
  {
    this->backend_.use();
  }

  template <typename L>
  void set_uniform_matrix_4fv(L &logger, char const *name, glm::mat4 const &matrix)
  {
    this->backend_.set_uniform_matrix_4fv(logger, name, matrix);
  }

  template <typename L>
  void set_uniform_array_4fv(L &logger, char const *name, std::array<float, 4> const &floats)
  {
    this->backend_.set_uniform_array_4fv(logger, name, floats);
  }

  template <typename L>
  void check_errors(L &logger)
  {
    this->check_opengl_errors(logger);
  }
  */
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
