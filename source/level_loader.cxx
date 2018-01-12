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

boost::optional<opengl::Color>
get_color(CppTable const& table, char const* name)
{
  auto const load_colors = table->template get_array_of<double>(name);
  if (!load_colors) {
    return boost::none;
  }

  auto const& c = *load_colors;
  opengl::Color color;
  color.r = c[0];
  color.g = c[1];
  color.b = c[2];
  color.a = c[3];
  return boost::make_optional(color);
}

boost::optional<glm::vec3>
get_vec3(CppTable const& table, char const* name)
{
  auto const load_data = table->template get_array_of<double>(name);
  if (!load_data) {
    return boost::none;
  }
  auto const& ld = *load_data;
  return glm::vec3{ld[0], ld[1], ld[2]};
}

glm::vec3
get_vec3_or_abort(CppTable const& table, char const* name)
{
  auto const vec3_data = get_vec3(table, name);
  if (!vec3_data) {
    std::abort();
  }
  return *vec3_data;
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
    auto const color_o = get_color(table, "color");
    if (color_o) {
      loader.set_color(*color_o);
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

  opengl::VertexAttribute
  get_copy_of_va(std::string const& va_name)
  {
    auto const cmp = [&va_name](auto const& it) { return it.first == va_name; };
    auto find_it = std::find_if(pair_.cbegin(), pair_.cend(), cmp);

    // TODO: for now assume we always find requested VA.
    assert(find_it != pair_.cend());
    return find_it->second;
  }
};

auto
load_textures(stlw::Logger &logger, CppTable const& config)
{
  opengl::TextureTable ttable;
  auto const load_texture = [&logger, &ttable](auto const& resource) {
    auto const name = get_string_or_abort(resource, "name");
    auto const type = get_string_or_abort(resource, "type");

    if (type == "texture:3dcube") {
      auto const front = get_string_or_abort(resource, "front");
      auto const right = get_string_or_abort(resource, "right");
      auto const back = get_string_or_abort(resource, "back");
      auto const left = get_string_or_abort(resource, "left");
      auto const top = get_string_or_abort(resource, "top");
      auto const bottom = get_string_or_abort(resource, "bottom");

      opengl::TextureFilenames texture_names{name, {front, right, back, left, top, bottom}};
      auto ti = opengl::texture::upload_3dcube_texture(logger, texture_names.filenames);
      ttable.add_texture(MOVE(texture_names), MOVE(ti));
    } else if (type == "texture:2d") {
      auto const filename = get_string_or_abort(resource, "filename");
      opengl::TextureFilenames texture_names{name, {filename}};
      auto ti = opengl::texture::allocate_texture(logger, texture_names.filenames[0]);
      ttable.add_texture(MOVE(texture_names), MOVE(ti));
    }
  };

  auto const resource_table = get_table_array_or_abort(config, "resource");
  std::for_each(resource_table->begin(), resource_table->end(), load_texture);
  return ttable;
}

auto
load_entities(stlw::Logger &logger, CppTable const& config)
{
  auto const load_entity = [](auto const& entity) {
    auto shader = get_string_or_abort(entity, "shader");
    auto pos = get_vec3_or_abort(entity, "pos");
    auto color = get_color(entity, "color");

    Transform transform;
    transform.translation = pos;
    return boomhs::EntityInfo{MOVE(transform), shader, color};
  };

  LoadedEntities entities;
  auto const entity_table = get_table_array(config, "entity");
  if (!entity_table) {
    return entities;
  }
  for (auto const& it : *entity_table) {
    auto entity = load_entity(it);
    entities.data.emplace_back(MOVE(entity));
  }
  return entities;
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

/*
void
upload_sp_textures(stlw::Logger &logger, ShaderPrograms &shader_programs,
    ResourceTable const& resource_table, CppTable const& table)
{
  auto const upload_texture = [&logger, &resource_table, &table](auto &shader_program) {
    auto const has_texture_o = get_bool(table, "has_texture");
    if (!has_texture_o) {
      return;
    }

    // find resource
    auto const filename = get_string_or_abort(table, "filename");
    auto const textures = resource_table.get_texture(filename);
    auto const& filenames = textures.filenames;
    if (textures.is_3dcube()) {
      auto texture = opengl::texture::upload_3dcube_texture(logger, filenames);
      shader_program.texture = boost::make_optional(MOVE(texture));
    } else if (textures.is_2d()) {
      auto texture = opengl::texture::allocate_texture(logger, filenames[0]);
      shader_program.texture = boost::make_optional(MOVE(texture));
    }
  };
  for (auto &pair : shader_programs) {
    auto &sp = pair.second;
    upload_texture(sp);
  }
}
*/

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

  auto const add_next_found = [&read_data](auto &apis, auto const& table, std::size_t const index) {
    auto data_o = read_data(table, index);
    bool const data_read = !!data_o;
    if (data_read) {
      auto data = MOVE(*data_o);
      apis.emplace_back(MOVE(data));
    }
    return data_read;
  };

  ParsedVertexAttributes pvas;
  auto const fn = [&pvas, &add_next_found](auto const& table) {
    auto const name = get_string_or_abort(table, "name");

    std::size_t i = 0u;
    std::vector<opengl::AttributePointerInfo> apis;
    while(add_next_found(apis, table, i++)) {}
    pvas.add(name, make_vertex_attribute(apis));
  };
  std::for_each((*vas_table_array).begin(), (*vas_table_array).end(), fn);
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

  auto const mesh_table = get_table_array_or_abort(area_config, "meshes");
  auto loader = opengl::ObjLoader{LOC::WHITE};
  auto meshes = load_meshes(loader, mesh_table);

  auto texture_table = load_textures(logger, area_config);
  auto entities = load_entities(logger, area_config);
  return Assets{MOVE(meshes), MOVE(shader_programs), MOVE(entities), MOVE(texture_table)};
}

} // ns boomhs
