#include <opengl/shader.hpp>
#include <opengl/debug.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>

#include <stlw/format.hpp>
#include <stlw/result.hpp>
#include <stlw/os.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{

using compiled_shader = stlw::ImplicitelyCastableMovableWrapper<GLuint, decltype(glDeleteShader)>;
using namespace opengl;


constexpr bool
is_invalid(GLuint const p) { return p == program_factory::INVALID_PROGRAM_ID(); }

inline bool
is_compiled(GLuint const handle)
{
  GLint is_compiled = GL_FALSE;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &is_compiled);
  return GL_FALSE != is_compiled;
}

inline void
gl_compile_shader(GLuint const handle, char const *source)
{
  GLint *p_length = nullptr;
  auto constexpr SHADER_COUNT = 1; // We're only compiling one shader (in this function anyway).
  glShaderSource(handle, SHADER_COUNT, &source, p_length);
  glCompileShader(handle);
}

template <typename T>
stlw::result<compiled_shader, std::string>
compile_shader(T const &data, GLenum const type)
{
  GLuint const handle = glCreateShader(type);
  gl_compile_shader(handle, data.c_str());

  // Check Vertex Shader
  if (true == is_compiled(handle)) {
    return compiled_shader{handle, glDeleteShader};
  }
  return stlw::make_error(global::log::get_shader_log(handle));
}

inline stlw::result<GLuint, std::string>
create_program()
{
  GLuint const program_id = glCreateProgram();
  if (0 == program_id) {
    return stlw::make_error(std::string{"GlCreateProgram returned 0."});
  }
  return program_id;
}

inline stlw::result<stlw::empty_type, std::string>
link_program(GLuint const program_id)
{
  // Link the program
  glLinkProgram(program_id);

  // Check the program
  GLint result;
  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  if (result == GL_FALSE) {
    return stlw::make_error("Linking the shader failed. Progam log '" +
                            global::log::get_program_log(program_id) + "'");
  }
  return stlw::make_empty();
}

struct AttributeVariableInfo {
  std::string variable;
};

struct VertexShaderInfo {
  std::string const& filename;
  std::string const& source;

  std::vector<AttributeVariableInfo> attribute_infos;
};

struct FragmentShaderInfo {
  std::string const& filename;
  std::string const& source;
};

stlw::result<GLuint, std::string>
compile_sources(VertexShaderInfo const &vertex_shader, FragmentShaderInfo const &fragment_shader)
{
  DO_TRY(auto const vertex_shader_id, compile_shader(vertex_shader.source, GL_VERTEX_SHADER));
  DO_TRY(auto const frag_shader_id, compile_shader(fragment_shader.source, GL_FRAGMENT_SHADER));
  DO_TRY(auto const program_id, create_program());

  std::cerr << fmt::format("compiling '{}'/'{}'\n", vertex_shader.filename, fragment_shader.filename);
  auto const& variable_infos = vertex_shader.attribute_infos;
  std::cerr << "number of variable infos: '" << variable_infos.size() << "'\n";
  FOR(i, variable_infos.size()) {
    auto const& vinfo = variable_infos[i];
    std::cerr << fmt::format("binding program_id: {}, name: {}, index: {}\n", program_id, vinfo.variable, i);
    glBindAttribLocation(program_id, i, vinfo.variable.c_str());
  }

  glAttachShader(program_id, vertex_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, vertex_shader_id); });

  glAttachShader(program_id, frag_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, frag_shader_id); });

  DO_EFFECT(link_program(program_id));
  std::cerr << "finished compiling\n";
  return program_id;
}

stlw::result<std::vector<AttributeVariableInfo>, std::string>
from_vertex_shader(std::string const& filename, std::string const& source)
{
  int idx = 0;
  std::string buffer;
  auto const make_error = [&](auto const& reason) {
    auto constexpr PREAMBLE = "Error parsing vertex shader for attribute information. Reason: '";
    auto constexpr SUFFIX = "'. Shader filename: '%s', line number: '%s', line string: '%s.";
    auto const error = PREAMBLE + std::string{reason} + SUFFIX;
    auto const fmt = fmt::sprintf(error, filename, std::to_string(idx), buffer);
    return stlw::make_error(fmt);
  };

  std::vector<AttributeVariableInfo> infos;
  std::istringstream iss(source.c_str());
  for (; std::getline(iss, buffer); ++idx) {
    auto constexpr IN_PREFIX = "in ";
    bool const begins_with_in_prefix = buffer.compare(0, ::strlen(IN_PREFIX), IN_PREFIX) == 0;
    if (!begins_with_in_prefix) {
      continue;
    }

    // The characters between the white-space and the semi-colon are the variable name.
    auto const semicolon_position = buffer.find(";", 0);
    if (semicolon_position == std::string::npos) {
        auto constexpr REASON = "No semi-colon found on variable line. ";
        return make_error(REASON);
    }

    // The character after the last whitespace before the semi-colon is the first character of the
    // input variable.
    auto const last_whitespace_position = buffer.find_last_of(" ", semicolon_position);
    if (last_whitespace_position == std::string::npos) {
      auto constexpr REASON = "No white-space found on input variable declaration. "
        "Unexpected syntax.";
      return make_error(REASON);
    }

    auto const start_pos = last_whitespace_position + 1;
    auto const length = semicolon_position - start_pos;
    auto variable_name = buffer.substr(start_pos, length);

    AttributeVariableInfo avi{MOVE(variable_name)};
    infos.emplace_back(MOVE(avi));

    // We don't have to parse whole shader, can stop at out variable declarations.
    auto constexpr OUT_PREFIX = "out ";
    bool const begins_with_out_variable = buffer.compare(0, ::strlen(OUT_PREFIX), OUT_PREFIX) == 0;
    if (begins_with_out_variable) {
      break;
    }
  }
  return infos;
}

std::string
attrib_type_to_string(GLenum const type)
{
  auto const& table = debug::attrib_to_string_table();
  auto const it = std::find_if(table.cbegin(), table.cend(), [&type](auto const& pair) { return pair.first == type; });
  assert(it != table.cend());

  auto const index = std::distance(table.cbegin(), it);
  return table[index].second;
}

std::string
uniform_type_to_string(GLenum const type)
{
  auto const& table = debug::uniform_to_string_table();
  auto const it = std::find_if(table.cbegin(), table.cend(), [&type](auto const& tuple) { return std::get<0>(tuple) == type; });
  assert(it != table.cend());

  auto const index = std::distance(table.cbegin(), it);
  auto const string = std::get<1>(table[index]);
  return string;
}

} // ns anonymous

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// grogram_factory
stlw::result<GLuint, std::string>
program_factory::from_files(vertex_shader_filename const v, fragment_shader_filename const f)
{
  auto const prefix = [](auto const &path) {
    return std::string{"./build-system/bin/shaders/"} + path;
  };
  auto const vertex_shader_path = prefix(v.filename);
  auto const fragment_shader_path = prefix(f.filename);

  // Read the Vertex/Fragment Shader code from ther file
  DO_TRY(auto const vertex_shader_source, stlw::read_file(vertex_shader_path));
  DO_TRY(auto attribute_variable_info, from_vertex_shader(vertex_shader_path, vertex_shader_source));
  DO_TRY(auto const fragment_source, stlw::read_file(fragment_shader_path));

  VertexShaderInfo const vertex_shader{vertex_shader_path, vertex_shader_source,
    MOVE(attribute_variable_info)};
  FragmentShaderInfo const fragment_shader{fragment_shader_path, fragment_source};

  return compile_sources(vertex_shader, fragment_shader);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ProgramHandle
ProgramHandle::ProgramHandle(GLuint const p)
  : program_(p)
{
}

ProgramHandle::ProgramHandle(ProgramHandle &&o)
  : program_(MOVE(o.program_))
{
  // We don't want to destroy the underlying program, we want to transfer the ownership to this
  // instance being moved into. This implements "handle-passing" allowing the user to observe
  // move-semantics for this object.
  o.program_ = program_factory::make_invalid();
}

ProgramHandle::~ProgramHandle()
{
  if (is_invalid(this->program_)) {
    glDeleteProgram(this->program_);
    this->program_ = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ShaderProgram
void
ShaderProgram::use_program(stlw::Logger &logger)
{
  glUseProgram(this->program_.handle());
  LOG_ANY_GL_ERRORS(logger, "Shader use/enable");
}

GLint
ShaderProgram::get_uniform_location(stlw::Logger &logger, GLchar const *name)
{
  global::log::clear_gl_errors();

  LOG_TRACE(fmt::sprintf("getting uniform '%s' location.", name));
  GLint const loc = glGetUniformLocation(this->program_.handle(), name);
  LOG_TRACE(fmt::sprintf("uniform '%s' found at '%d'.", name, loc));

  LOG_ANY_GL_ERRORS(logger, "get_uniform_location");
  assert(-1 != loc);
  return loc;
}

void
ShaderProgram::set_uniform_matrix_3fv(stlw::Logger &logger, GLchar const *name, glm::mat4 const &matrix)
{
  use_program(logger);

  auto const loc = get_uniform_location(logger, name);
  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // count:
  // For the matrix (glUniformMatrix*) commands, specifies the number of matrices that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array of matrices, and 1 or more
  // if it is an array of matrices.
  GLsizei constexpr COUNT = 1;
  GLboolean constexpr TRANSPOSE_MATRICES = GL_FALSE;

  LOG_TRACE(fmt::sprintf("sending uniform mat3 at loc '%d' with data '%s' to GPU", loc,
        glm::to_string(matrix)));
  glUniformMatrix3fv(loc, COUNT, TRANSPOSE_MATRICES, glm::value_ptr(matrix));
  LOG_ANY_GL_ERRORS(logger, "set_uniform_matrix_3fv");
}

void
ShaderProgram::set_uniform_matrix_4fv(stlw::Logger &logger, GLchar const *name, glm::mat4 const &matrix)
{
  use_program(logger);

  auto const loc = get_uniform_location(logger, name);
  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // count:
  // For the matrix (glUniformMatrix*) commands, specifies the number of matrices that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array of matrices, and 1 or more
  // if it is an array of matrices.
  GLsizei constexpr COUNT = 1;
  GLboolean constexpr TRANSPOSE_MATRICES = GL_FALSE;

  LOG_TRACE(fmt::sprintf("sending uniform matrix at loc '%d' with data '%s' to GPU", loc,
        glm::to_string(matrix)));
  glUniformMatrix4fv(loc, COUNT, TRANSPOSE_MATRICES, glm::value_ptr(matrix));
  LOG_ANY_GL_ERRORS(logger, "set_uniform_matrix_4fv");
}

void
ShaderProgram::set_uniform_array_4fv(stlw::Logger &logger, GLchar const *name, std::array<float, 4> const &floats)
{
  use_program(logger);

  auto const loc = get_uniform_location(logger, name);
  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // For the vector (glUniform*v) commands, specifies the number of elements that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array, and 1 or more if it is an
  // array.
  GLsizei constexpr COUNT = 1;

  glUniform4fv(loc, COUNT, floats.data());
  LOG_ANY_GL_ERRORS(logger, "set_uniform_array_4fv");
}

void
ShaderProgram::set_uniform_array_3fv(stlw::Logger &logger, GLchar const* name, std::array<float, 3> const& array)
{
  use_program(logger);

  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // For the vector (glUniform*v) commands, specifies the number of elements that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array, and 1 or more if it is an
  // array.
  GLsizei constexpr COUNT = 1;

  auto const loc = get_uniform_location(logger, name);
  glUniform3fv(loc, COUNT, array.data());
  LOG_ANY_GL_ERRORS(logger, "set_uniform_array_3fv");
}

void
ShaderProgram::set_uniform_float1(stlw::Logger &logger, GLchar const* name, float const value)
{
  use_program(logger);

  auto const loc = get_uniform_location(logger, name);
  glUniform1f(loc, value);
  LOG_ANY_GL_ERRORS(logger, "glUniform1f");
}

void
ShaderProgram::set_uniform_int1(stlw::Logger &logger, GLchar const* name, int const value)
{
  use_program(logger);

  auto const loc = get_uniform_location(logger, name);
  glUniform1i(loc, value);
  LOG_ANY_GL_ERRORS(logger, "glUniform1i");
}

void
ShaderProgram::set_uniform_bool(stlw::Logger &logger, GLchar const* name, bool const value)
{
  use_program(logger);

  auto const loc = get_uniform_location(logger, name);
  glUniform1i(loc, static_cast<int>(value));
  LOG_ANY_GL_ERRORS(logger, "glUniform1i");
}

std::string
glchar_ptr_to_string(GLchar const* ptr)
{
  char const* cstring = static_cast<char const*>(ptr);
  return std::string{cstring};
}

void
print_active_attributes(std::ostream &stream, GLuint const program)
{
  GLint buffer_size{0};
  glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &buffer_size);

  GLint count{0};
  glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);

  GLsizei length{0};
  GLint size{0};
  GLenum type{0};

  GLchar name[buffer_size];
  stlw::memzero(name, buffer_size);

  stream << "Active Attributes: " << std::to_string(count) << "\n";
  FORI(i, count) {
    glGetActiveAttrib(program, static_cast<GLuint>(i), buffer_size, &length, &size, &type, name);

    auto const type_string = attrib_type_to_string(type);
    auto const name_string = glchar_ptr_to_string(name);
    stream << fmt::format("Attribute #{} Type: {} Name: {}\n", i, type_string, name_string);
  }
}

void
print_active_uniforms(std::ostream &stream, GLuint const program)
{
  GLint buffer_size{0};
  glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &buffer_size);

  GLint count{0};
  glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

  GLsizei length{0};
  GLint size{0};
  GLenum type{0};

  GLchar name[buffer_size];
  stlw::memzero(name, buffer_size);

  stream << "Active Uniforms: '" << std::to_string(count) << "'\n";
  FORI(i, count) {
    glGetActiveUniform(program, static_cast<GLuint>(i), buffer_size, &length, &size, &type, name);

    auto const type_string = uniform_type_to_string(type);
    auto const name_string = glchar_ptr_to_string(name);

    stream << fmt::format("Uniform #{} Type: {} Name: {}\n", i, type_string, name_string);
  }
}

std::ostream&
operator<<(std::ostream &stream, ShaderProgram const& sp)
{
  auto const& program = sp.handle();
  print_active_attributes(stream, program);
  print_active_uniforms(stream, program);
  return stream;
}

} // ns opengl
