#include <extlibs/glew.hpp>
#include <gl_sdl/gl_sdl_log.hpp>
#include <opengl/debug.hpp>
#include <opengl/global.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

#include <boomhs/math.hpp>
#include <common/algorithm.hpp>
#include <common/os.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <cstring>
#include <extlibs/fmt.hpp>

namespace
{

using compiled_shader = common::ImplicitelyCastableMovableWrapper<GLuint, decltype(glDeleteShader)>;
using namespace opengl;

auto constexpr INVALID_PROGRAM_ID = 0;

bool
is_compiled(GLuint const handle)
{
  GLint is_compiled = GL_FALSE;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &is_compiled);
  return GL_FALSE != is_compiled;
}

Result<compiled_shader, std::string>
compile_shader(common::Logger& logger, GLenum const type, std::string const& data)
{
  GLuint const handle = glCreateShader(type);

  char const* source = data.data();
  glShaderSource(handle, 1, &source, nullptr);
  LOG_ANY_GL_ERRORS(logger, "glShaderSource");

  glCompileShader(handle);
  LOG_ANY_GL_ERRORS(logger, "glCompileShader");

  // Check Vertex Shader
  if (true == is_compiled(handle)) {
    return Ok(compiled_shader{handle, glDeleteShader});
  }
  return Err(gl_sdl::get_shader_log(handle));
}

inline Result<GLuint, std::string>
create_program()
{
  GLuint const program_id = glCreateProgram();
  if (INVALID_PROGRAM_ID == program_id) {
    return Err(std::string{"GlCreateProgram returned 0."});
  }
  return Ok(program_id);
}

inline Result<common::none_t, std::string>
link_program(common::Logger& logger, GLuint const program_id)
{
  // Link the program
  LOG_ANY_GL_ERRORS(logger, "glLinkProgram before");
  glLinkProgram(program_id);
  LOG_ANY_GL_ERRORS(logger, "glLinkProgram after");

  // Check the program
  GLint result;
  glGetProgramiv(program_id, GL_LINK_STATUS, &result);
  if (result == GL_FALSE) {
    auto const program_log = gl_sdl::get_program_log(program_id);
    auto const shader_log  = gl_sdl::get_shader_log(program_id);
    auto const fmt = fmt::sprintf("Linking the shader failed. Progam log '%s'. Shader Log '%s'",
                                  program_log, shader_log);
    return Err(fmt);
  }
  return OK_NONE;
}

struct AttributeVariableInfo
{
  std::string variable;
};

struct VertexShaderInfo
{
  std::string const& filename;
  std::string const& source;

  std::vector<AttributeVariableInfo> attribute_infos;
};

struct FragmentShaderInfo
{
  std::string const& filename;
  std::string const& source;
};

Result<std::vector<AttributeVariableInfo>, std::string>
from_vertex_shader(std::string const& filename, std::string const& source)
{
  int         idx = 0;
  std::string buffer;
  auto const  make_error = [&](auto const& reason) {
    auto constexpr PREAMBLE = "Error parsing vertex shader for attribute information. Reason: '";
    auto constexpr SUFFIX   = "'. Shader filename: '%s', line number: '%s', line string: '%s.";
    auto const error        = PREAMBLE + std::string{reason} + SUFFIX;
    auto const fmt          = fmt::sprintf(error, filename, std::to_string(idx), buffer);
    return Err(fmt);
  };

  std::vector<AttributeVariableInfo> infos;
  std::istringstream                 iss(source.c_str());
  for (; std::getline(iss, buffer); ++idx) {
    auto constexpr IN_PREFIX         = "in ";
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

    auto const start_pos     = last_whitespace_position + 1;
    auto const length        = semicolon_position - start_pos;
    auto       variable_name = buffer.substr(start_pos, length);

    AttributeVariableInfo avi{MOVE(variable_name)};
    infos.emplace_back(MOVE(avi));

    // We don't have to parse whole shader, can stop at out variable declarations.
    auto constexpr OUT_PREFIX           = "out ";
    bool const begins_with_out_variable = buffer.compare(0, ::strlen(OUT_PREFIX), OUT_PREFIX) == 0;
    if (begins_with_out_variable) {
      break;
    }
  }
  return Ok(MOVE(infos));
}

std::string
attrib_type_to_string(GLenum const type)
{
  auto const& table = debug::attrib_to_string_table();
  auto const  it    = std::find_if(table.cbegin(), table.cend(),
                               [&type](auto const& pair) { return pair.first == type; });
  assert(it != table.cend());

  auto const index = std::distance(table.cbegin(), it);
  return table[index].second;
}

std::string
uniform_type_to_string(GLenum const type)
{
  auto const& table = debug::uniform_to_string_table();
  auto const  it    = std::find_if(table.cbegin(), table.cend(),
                               [&type](auto const& tuple) { return std::get<0>(tuple) == type; });
  assert(it != table.cend());

  auto const index  = std::distance(table.cbegin(), it);
  auto const string = std::get<1>(table[index]);
  return string;
}

std::string
glchar_ptr_to_string(GLchar const* ptr)
{
  char const* cstring = static_cast<char const*>(ptr);
  return std::string{cstring};
}

auto
active_attributes_string(GLuint const program)
{
  GLint buffer_size{0};
  glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &buffer_size);

  GLint count{0};
  glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);

  GLsizei length{0};
  GLint   size{0};
  GLenum  type{0};

  GLchar name[buffer_size];
  common::memzero(name, buffer_size);

  std::string result;
  result += "Active Attributes: {" + std::to_string(count) + "}(";
  FORI(i, count)
  {
    if (i > 0) {
      result += ", ";
    }
    glGetActiveAttrib(program, static_cast<GLuint>(i), buffer_size, &length, &size, &type, name);

    auto const type_string = attrib_type_to_string(type);
    auto const name_string = glchar_ptr_to_string(name);
    result += fmt::sprintf("[attr: %i, name: %s, type: %s]", i, name_string, type_string);
  }
  result += ")";
  return result;
}

std::string
active_uniforms_string(GLuint const program)
{
  GLint buffer_size{0};
  glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &buffer_size);

  GLint count{0};
  glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

  GLsizei length{0};
  GLint   size{0};
  GLenum  type{0};

  GLchar name[buffer_size];
  common::memzero(name, buffer_size);

  std::string result;
  result += "Number Active Uniforms: {" + std::to_string(count) + "}(";
  FORI(i, count)
  {
    if (i > 0) {
      result += ", ";
    }
    glGetActiveUniform(program, static_cast<GLuint>(i), buffer_size, &length, &size, &type, name);

    auto const type_string = uniform_type_to_string(type);
    auto const name_string = glchar_ptr_to_string(name);

    result += fmt::sprintf("[uniform: %i name: %s, type: %s]", i, name_string, type_string);
  }
  result += ")";
  return result;
}

} // namespace

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// program_factory
Result<GLuint, std::string>
program_factory::from_sources(common::Logger& logger, char const* v, char const* f)
{
  auto const vertex_shader_id = TRY_MOVEOUT(compile_shader(logger, GL_VERTEX_SHADER, v));
  auto const frag_shader_id   = TRY_MOVEOUT(compile_shader(logger, GL_FRAGMENT_SHADER, f));
  auto const program_id       = TRY_MOVEOUT(create_program());

  auto attribute_variable_info = TRY_MOVEOUT(from_vertex_shader(v, f));

  FOR(i, attribute_variable_info.size())
  {
    auto const& vinfo = attribute_variable_info[i];
    LOG_DEBUG_FMT("binding program_id: {}, name: {}, index: {}", program_id, vinfo.variable, i);
    glBindAttribLocation(program_id, i, vinfo.variable.c_str());
  }

  glAttachShader(program_id, vertex_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, vertex_shader_id); });

  glAttachShader(program_id, frag_shader_id);
  ON_SCOPE_EXIT([&]() { glDetachShader(program_id, frag_shader_id); });

  DO_EFFECT(link_program(logger, program_id));
  LOG_TRACE("finished compiling");
  return Ok(program_id);
}

Result<GLuint, std::string>
program_factory::from_files(common::Logger& logger, VertexShaderFilename const& v,
                            FragmentShaderFilename const& f)
{
  auto const prefix = [](auto const& path) {
    return std::string{"./build-system/bin/shaders/"} + path;
  };
  auto const vertex_shader_path   = prefix(v.filename());
  auto const fragment_shader_path = prefix(f.filename());

  // Read the Vertex/Fragment Shader code from ther file
  auto const vertex_shader_source = TRY_MOVEOUT(common::read_file(vertex_shader_path));

  auto attribute_variable_info =
      TRY_MOVEOUT(from_vertex_shader(vertex_shader_path, vertex_shader_source));
  auto const fragment_shader_source = TRY_MOVEOUT(common::read_file(fragment_shader_path));

  LOG_TRACE_SPRINTF("Compiling shaders vert: %s, frag: %s", v.filename(), f.filename());
  return from_sources(logger, vertex_shader_source.c_str(), fragment_shader_source.c_str());
}

Result<GLuint, std::string>
program_factory::from_sources(common::Logger& logger, std::string const& a, std::string const& b)
{
  return from_sources(logger, a.c_str(), b.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ProgramHandle
ProgramHandle::ProgramHandle(GLuint const p)
    : program_(p)
{
  // Initially when a ProgramHandle is constructed from a GLuint, the ProgramHandle "assumes
  // ownership", or will assume the responsibility of deleting the underlying opengl program.
  assert(p != INVALID_PROGRAM_ID);
}

ProgramHandle::ProgramHandle(ProgramHandle&& other)
    : program_(MOVE(other.program_))
{
  // The "moved-from" handle no longer has the responsibility of freeing the underlying opengl
  // program.
  other.program_ = INVALID_PROGRAM_ID;

#ifdef DEBUG_BUILD
  debug_check = MOVE(other.debug_check);
#endif
}

ProgramHandle::~ProgramHandle()
{
  DEBUG_ASSERT_NOT_BOUND(*this);

  if (program_ != INVALID_PROGRAM_ID) {
    glDeleteProgram(program_);
    program_ = INVALID_PROGRAM_ID;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ShaderProgram
void
ShaderProgram::bind_impl(common::Logger& logger)
{
  glUseProgram(program_.handle());
  LOG_ANY_GL_ERRORS(logger, "Shader use/enable");
}

void
ShaderProgram::unbind_impl(common::Logger& logger)
{
#ifdef DEBUG_BUILD
  glUseProgram(0);
#endif
}

GLint
ShaderProgram::get_uniform_location(common::Logger& logger, GLchar const* name)
{
  DEBUG_ASSERT_BOUND(*this);
  LOG_DEBUG_SPRINTF("getting uniform '%s' location.", name);
  GLint const loc = glGetUniformLocation(program_.handle(), name);
  LOG_DEBUG_SPRINTF("uniform '%s' found at '%d'.", name, loc);

  LOG_ANY_GL_ERRORS(logger, "get_uniform_location");
  assert(-1 != loc);
  return loc;
}

std::string
ShaderProgram::to_string() const
{
  auto const& handle     = this->handle();
  auto const  attributes = active_attributes_string(handle);
  auto const  uniforms   = active_uniforms_string(handle);

#ifdef DEBUG_BUILD
  auto const& v = source_paths_.vertex;
  auto const& f = source_paths_.fragment;
  return fmt::sprintf("ShaderProgram (vs): '%s' fs: '%s' attributes: %s",
                      v.filename(),
                      f.filename(),
                      attributes);
  // + uniforms);
#else
  return fmt::sprintf("ShaderProgram: %s", attributes); // + uniforms);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ShaderPrograms
std::vector<std::string>
ShaderPrograms::all_shader_names() const
{
  std::vector<std::string> result;
  for (auto const& it : shader_programs_) {
    result.emplace_back(it.first);
  }
  return result;
}

std::string
ShaderPrograms::all_shader_names_flattened(char const delim) const
{
  auto const        sn = all_shader_names();
  std::stringstream buffer;

  for (auto const& it : sn) {
    buffer << it;
    buffer << delim;
  }
  return buffer.str();
}

std::optional<size_t>
ShaderPrograms::index_of_nickname(std::string const& name) const
{
  auto const sn = all_shader_names();
  size_t     i  = 0;
  for (auto const& it : sn) {
    if (it == name) {
      return std::make_optional(i);
    }
    ++i;
  }
  return std::nullopt;
}

std::optional<std::string>
ShaderPrograms::nickname_at_index(size_t const index) const
{
  auto const sn = all_shader_names();
  if (index >= sn.size()) {
    return std::nullopt;
  }
  return sn[index];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Free Functions
Result<ShaderProgram, std::string>
make_shader_program(common::Logger& logger, char const* vs, char const* fs, VertexAttribute&& va)
{
  VertexShaderFilename   v{vs};
  FragmentShaderFilename f{fs};
  auto                   sp = TRY_MOVEOUT(program_factory::from_files(logger, v, f));
  return Ok(ShaderProgram{ProgramHandle{sp}, MOVE(va)
#ifdef DEBUG_BUILD
                                                 ,
                          PathToShaderSources{MOVE(v), MOVE(f)}
#endif
                          });
}

Result<ShaderProgram, std::string>
make_shader_program(common::Logger& logger, std::string const& vs, std::string const& fs,
                    VertexAttribute&& va)
{
  return make_shader_program(logger, vs.c_str(), fs.c_str(), MOVE(va));
}

Result<ShaderProgram, std::string>
make_shader_program(common::Logger& logger, char const* vs, char const* fs,
                    AttributePointerInfo&& api)
{
  auto va = make_vertex_attribute(MOVE(api));
  return make_shaderprogram_fromsources(logger, vs, fs, MOVE(va));
}

Result<ShaderProgram, std::string>
make_shader_program(common::Logger& logger, std::string const& vs, std::string const& fs,
                    AttributePointerInfo&& api)
{
  return make_shader_program(logger, vs.c_str(), fs.c_str(), MOVE(api));
}

ShaderProgram
make_shaderprogram_expect(common::Logger& logger, char const* vs, char const* fs,
                          AttributePointerInfo&& api)
{
  return make_shader_program(logger, vs, fs, MOVE(api))
      .expect_moveout(fmt::sprintf("Error loading '%s'/'%s' shader program", vs, fs));
}

ShaderProgram
make_shaderprogram_expect(common::Logger& logger, std::string const& vs, std::string const& fs,
                          AttributePointerInfo&& api)
{
  return make_shaderprogram_expect(logger, vs.c_str(), fs.c_str(), MOVE(api));
}


Result<ShaderProgram, std::string>
make_shaderprogram_fromsources(common::Logger& logger, char const* vs, char const* fs,
                               VertexAttribute&& va)
{
  auto program = TRY_MOVEOUT(program_factory::from_sources(logger, vs, fs));

  VertexShaderFilename     vfname{"SOURCE CODE"};
  FragmentShaderFilename   ffname{"SOURCE CODE"};
  return Ok(ShaderProgram{ProgramHandle{program}, MOVE(va)
#ifdef DEBUG_BUILD
                                           ,
                          PathToShaderSources{MOVE(vfname), MOVE(ffname)}
#endif
                          });
}


Result<ShaderProgram, std::string>
make_shaderprogram_fromsources(common::Logger& logger, std::string const& vs, std::string const& fs,
                               VertexAttribute&& va)
{
  return make_shader_program(logger, vs.c_str(), fs.c_str(), MOVE(va));
}


std::ostream&
operator<<(std::ostream& stream, ShaderProgram const& sp)
{
  stream << sp.to_string();
  return stream;
}

} // namespace opengl
