#include <boomhs/level_loader.hpp>
#include <boomhs/assets.hpp>
#include <opengl/obj.hpp>
#include <stlw/result.hpp>

using namespace boomhs;
using namespace opengl;

#define TRY_OPTION_GENERAL_EVAL(VAR_DECL, V, expr)                                                 \
  auto V{expr};                                                                                    \
  if (!V) {                                                                                        \
    return cpptoml::option<opengl::AttributePointerInfo>{}; \
  }                                                                                                \
  VAR_DECL{MOVE(V)};

#define TRY_OPTION_CONCAT(VAR_DECL, TO_CONCAT, expr)                                               \
  TRY_OPTION_GENERAL_EVAL(VAR_DECL, _TRY_OPTION_TEMPORARY_##TO_CONCAT, expr)

#define TRY_OPTION_EXPAND_VAR(VAR_DECL, to_concat, expr)                                           \
  TRY_OPTION_CONCAT(VAR_DECL, to_concat, expr)

// TRY_OPTION
#define TRY_OPTION(VAR_DECL, expr)                                                                 \
  TRY_OPTION_EXPAND_VAR(VAR_DECL, __COUNTER__, expr)

namespace
{

CppTableArray
get_table_array(CppTable const& config, char const* name)
{
  return config->get_table_array(name);
}

CppTableArray
get_table_array_or_abort(CppTable const& config, char const* name)
{
  auto table_o = get_table_array(config, name);
  if (!table_o) {
    std::abort();
  }
  return table_o;
}

template<typename T>
boost::optional<T>
get_value(CppTable const& table, char const* name)
{
  auto cpptoml_option = table->get_as<T>(name);
  if (!cpptoml_option) {
    return boost::none;
  }
  // Move value out of cpptoml and into boost::optional
  auto value = cpptoml_option.move_out();
  return boost::make_optional(MOVE(value));
}

auto
get_string(CppTable const& table, char const* name)
{
  return get_value<std::string>(table, name);
}

auto
get_bool(CppTable const& table, char const* name)
{
  return get_value<bool>(table, name);
}

boost::optional<GLsizei>
get_sizei(CppTable const& table, char const* name)
{
  return get_value<GLsizei>(table, name);
}

template<typename T>
auto
get_or_abort(CppTable const& table, char const* name)
{
  auto o = table->get_as<T>(name);
  if (!o) {
    std::abort();
  }
  return *o;
}

std::string
get_string_or_abort(CppTable const& table, char const* name)
{
  return get_or_abort<std::string>(table, name);
}

auto
get_bool_or_abort(CppTable const& table, char const* name)
{
  return get_or_abort<bool>(table, name);
}

auto
load_meshes(opengl::ObjLoader &loader, CppTableArray const& mesh_table)
{
  auto const load = [&loader](auto const& table) {
    auto const name = get_string_or_abort(table, "name");

    auto const normals = opengl::LoadNormals{get_bool_or_abort(table, "normals")};
    auto const uvs = opengl::LoadUvs{get_bool_or_abort(table, "uvs")};

    auto const obj = "assets/" + name + ".obj";
    auto const mtl = "assets/" + name + ".mtl";

    // cpptoml (not sure if TOML in general) won't let me have an array of floats,
    // so an array of doubles (buffer) is used to read from the file.
    auto const load_colors = table->template get_array_of<double>("color");
    if (load_colors) {
      opengl::Color color;
      color.r = (*load_colors)[0];
      color.g = (*load_colors)[1];
      color.b = (*load_colors)[2];
      color.a = (*load_colors)[3];
      loader.set_color(color);
    }
    auto mesh = loader.load_mesh(obj.c_str(), mtl.c_str(), normals, uvs);
    return std::make_pair(name, MOVE(mesh));
  };
  ObjCache cache;
  for (auto const& table : *mesh_table) {
    auto pair = load(table);
    cache.add_obj(pair.first, MOVE(pair.second));
  }
  return cache;
}

class ParsedVertexAttributes
{
  using pair_t = std::pair<std::string, opengl::VertexAttribute>;
  std::vector<pair_t> pair_;
public:
  ParsedVertexAttributes() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ParsedVertexAttributes);

  void
  add(std::string const& name, opengl::VertexAttribute &&va)
  {
    auto pair = std::make_pair(name, MOVE(va));
    pair_.emplace_back(MOVE(pair));
  }

  auto
  get_copy_of_va(std::string const& va_name)
  {
    auto const cmp = [&va_name](auto const& it) { return it.first == va_name; };
    auto find_it = std::find_if(pair_.cbegin(), pair_.cend(), cmp);

    // TODO: for now assume we always find requested VA.
    assert(find_it != pair_.cend());
    return find_it->second;
  }
};

struct TextureFilenames
{
  std::string name;
  std::vector<std::string> filenames;

  auto num_filenames() const { return filenames.size(); }

  // TODO: should this check be made an explicit enum or something somewhere?
  bool is_3dcube() const { return filenames.size() == 6; }
  bool is_2d() const { return filenames.size() == 1; }
};

class ResourceTable
{
  std::vector<TextureFilenames> filenames_;
public:
  ResourceTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ResourceTable);

  void
  add_texture(TextureFilenames &&f)
  {
    filenames_.emplace_back(MOVE(f));
  }

  TextureFilenames
  get_texture(std::string const& name) const
  {
    auto const cmp = [&name](auto const& it) { return it.name == name; };
    auto const find_it = std::find_if(filenames_.cbegin(), filenames_.cend(), cmp);

    // TODO: for now assume always find texture
    assert(find_it != filenames_.cend());
    return *find_it;
  }
};

auto
load_resources(stlw::Logger &logger, CppTable const& config)
{
  ResourceTable rtable;
  auto const resource_table = get_table_array_or_abort(config, "resource");

  for (auto const& resource : *resource_table) {
    auto const name = get_string_or_abort(resource, "name");
    auto const type = get_string_or_abort(resource, "type");

    // TODO: support other types than 3dcube texture's
    assert(type == "texture:3dcube");

    auto const front = get_string_or_abort(resource, "front");
    auto const right = get_string_or_abort(resource, "right");
    auto const back = get_string_or_abort(resource, "back");
    auto const left = get_string_or_abort(resource, "left");
    auto const top = get_string_or_abort(resource, "top");
    auto const bottom = get_string_or_abort(resource, "bottom");

    TextureFilenames texture_names{name, {front, right, back, left, top, bottom}};
    rtable.add_texture(MOVE(texture_names));
  }
  return rtable;
}

using LoadResult = stlw::result<std::pair<std::string, opengl::ShaderProgram>, std::string>;
LoadResult
load_shader(stlw::Logger &logger, ParsedVertexAttributes &pvas, CppTable const& table)
{
  auto const name = get_string_or_abort(table, "name");
  auto const vertex = get_string_or_abort(table, "vertex");
  auto const fragment = get_string_or_abort(table, "fragment");
  auto const va_name = get_string_or_abort(table, "va");

  // TODO: ugly hack, maybe think about...
  auto va = pvas.get_copy_of_va(va_name);
  DO_TRY(auto program, opengl::make_shader_program(vertex, fragment, MOVE(va)));

  program.is_skybox = get_bool(table, "is_skybox").get_value_or(false);
  program.instance_count = get_sizei(table, "instance_count");

  program.receives_light = get_bool(table, "receives_light").get_value_or(false);
  program.is_lightsource = get_bool(table, "is_lightsource").get_value_or(false);

  return std::make_pair(name, MOVE(program));
}

stlw::result<opengl::ShaderPrograms, std::string>
load_shaders(stlw::Logger &logger, ParsedVertexAttributes &&pvas, CppTable const& config)
{
  auto const shaders_table = get_table_array_or_abort(config, "shaders");
  opengl::ShaderPrograms sps;
  for (auto const& shader_table : *shaders_table) {
    DO_TRY(auto pair, load_shader(logger, pvas, shader_table));
    sps.add(pair.first, MOVE(pair.second));
  }
  return sps;
}

void
upload_sp_textures(stlw::Logger &logger, ShaderPrograms &shader_programs,
    ResourceTable const& resource_table, CppTable const& table)
{
  auto const upload_texture = [&logger, &resource_table, &table](auto &shader_program) {
    auto const texture_name = get_string(table, "has_texture");
    if (texture_name) {
      // find resource
      auto const textures = resource_table.get_texture(*texture_name);
      auto const& filenames = textures.filenames;
      if (textures.is_3dcube()) {
        auto texture = opengl::texture::upload_3dcube_texture(logger, filenames);
        shader_program.texture = boost::make_optional(MOVE(texture));
      } else if (textures.is_2d()) {
        auto texture = opengl::texture::allocate_texture(logger, filenames[0]);
        shader_program.texture = boost::make_optional(MOVE(texture));
      }
    }
  };
  for (auto &pair : shader_programs) {
    auto &sp = pair.second;
    upload_texture(sp);
  }
}

auto
load_vas(CppTable const& config)
{
  auto vas_table_array = config->get_table_array("vas");
  assert(vas_table_array);

  auto const read_data = [&](auto const& table, std::size_t const index) {
    auto const dataname = "data" + std::to_string(index);

    // THINKING EXPLAINED:
    // See if there is a data field, if there isn't no problem return (TRY_OPTION)
    //
    // Otherwise if there is a data field, require both the "type" and "num" fields,
    // as otherwise this indicates a malformed-field.
    TRY_OPTION(auto data_table, table->get_table(dataname));
    auto const type_s = get_string_or_abort(data_table, "type");

    // TODO: FOR NOW, only support floats. Easy to implement rest
    assert("float" == type_s);
    auto const type = GL_FLOAT;
    auto const num = get_or_abort<int>(data_table, "num");

    auto const uint_index = static_cast<GLuint>(index);
    auto api = opengl::AttributePointerInfo{uint_index, type, num};
    return cpptoml::option<opengl::AttributePointerInfo>{MOVE(api)};
  };

  auto const add_next_found = [&](auto &apis, auto const& va_table, std::size_t const index) {
    auto data_o = read_data(va_table, index);
    bool const data_read = !!data_o;
    if (data_read) {
      auto data = MOVE(*data_o);
      apis.emplace_back(MOVE(data));
    }
    return data_read;
  };

  ParsedVertexAttributes pvas;
  for(auto const& va_table : *vas_table_array) {
    auto const name = get_string_or_abort(va_table, "name");

    std::size_t i = 0u;
    std::vector<opengl::AttributePointerInfo> apis;
    while(add_next_found(apis, va_table, i++)) {}
    pvas.add(name, make_vertex_attribute(apis));
  }
  return pvas;
}

} // ns anon

namespace boomhs
{

stlw::result<Assets, std::string>
load_assets(stlw::Logger &logger)
{
  CppTable engine_config = cpptoml::parse_file("engine.toml");
  assert(engine_config);

  ParsedVertexAttributes pvas = load_vas(engine_config);
  DO_TRY(auto shader_programs, load_shaders(logger, MOVE(pvas), engine_config));

  CppTable area_config = cpptoml::parse_file("levels/area0.toml");
  assert(area_config);

  auto const resource_table = load_resources(logger, area_config);
  upload_sp_textures(logger, shader_programs, resource_table, area_config);

  auto const mesh_table = get_table_array_or_abort(area_config, "meshes");
  auto loader = opengl::ObjLoader{LOC::WHITE};
  auto meshes = load_meshes(loader, mesh_table);

  return Assets{MOVE(meshes), MOVE(shader_programs)};
}

} // ns boomhs
