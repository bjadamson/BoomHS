#pragma once
#include <glm/glm.hpp>
#include <stlw/type_macros.hpp>

namespace gfx
{

template<typename B>
class pipeline
{
  B backend_;

  explicit pipeline(B &&b)
    : backend_(std::move(b))
  {
  }

  template<typename T>
  friend pipeline<T> make;
public:
  MOVE_CONSTRUCTIBLE_ONLY(pipeline);

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

template<typename B>
class pipeline_factory
{
  B &backend_factory_;
  MOVE_CONSTRUCTIBLE_ONLY(pipeline_factory);
public:
  explicit pipeline_factory(B &bf)
    : backend_factory_(bf)
  {
  }

  auto
  make(vertex_shader_filename const& v, fragment_shader_filename const& f)
  {
    return pipeline<B>{this->backend_factory_.make(v, f)};
  }
};

} // ns gfx
