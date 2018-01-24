#include <boomhs/level_loader.hpp>
#include <boomhs/assets.hpp>
#include <boomhs/components.hpp>
#include <opengl/obj.hpp>

#include <stlw/result.hpp>

#include <boost/algorithm/string/predicate.hpp>

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

  std::vector<double> const& c = *load_colors;
  opengl::Color color;
  color.set_r(c[0]);
  color.set_g(c[1]);
  color.set_b(c[2]);
  color.set_a(c[3]);
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
load_entities(stlw::Logger &logger, CppTable const& config, TextureTable const& ttable,
    entt::DefaultRegistry &registry)
{
  auto const load_entity = [&registry, &ttable](auto const& file) {
    // clang-format off
    auto shader =           get_string_or_abort(file, "shader");
    auto geometry =         get_string_or_abort(file, "geometry");
    auto pos =              get_vec3_or_abort(file,   "pos");
    auto scale_o =          get_vec3(file,            "scale");
    auto rotation_o =       get_vec3(file,            "rotation");
    auto color =            get_color(file,           "color");
    auto texture_name =     get_string(file,          "texture");
    auto pointlight_o =     get_vec3(file,            "pointlight");
    auto player =           get_string(file,          "player");
    auto receives_light_o = get_bool(file,            "receives_light");
    // clang-format on

    // texture OR color fields, not both
    assert((!color && !texture_name) || (!color && texture_name) || (color && !texture_name));

    auto entity = registry.create();
    auto &transform = registry.assign<Transform>(entity);
    transform.translation = pos;

    if (scale_o) {
      transform.scale = *scale_o;
    }
    if (rotation_o) {
      // TODO: simplify
      glm::vec3 const rotation = *rotation_o;
      auto const x_rotation = glm::angleAxis(glm::radians(rotation.x), opengl::X_UNIT_VECTOR);
      auto const y_rotation = glm::angleAxis(glm::radians(rotation.y), opengl::Y_UNIT_VECTOR);
      auto const z_rotation = glm::angleAxis(glm::radians(rotation.z), opengl::Z_UNIT_VECTOR);
      transform.rotation = x_rotation * transform.rotation;
      transform.rotation = y_rotation * transform.rotation;
      transform.rotation = z_rotation * transform.rotation;
    }

    auto &sn = registry.assign<ShaderName>(entity);
    sn.value = shader;

    if (player) {
      registry.assign<Player>(entity);
    }
    if (geometry == "cube") {
      registry.assign<CubeRenderable>(entity);
    }
    else if (geometry == "skybox") {
      registry.assign<SkyboxRenderable>(entity);
    }
    else if (boost::starts_with(geometry, "mesh")) {
      auto const parse_meshname = [](auto const& field) {
        auto const len = ::strlen("mesh:");
        assert(0 < len);
        return field.substr(len, field.length() - len);
      };
      auto &meshc = registry.assign<MeshRenderable>(entity);
      meshc.name = parse_meshname(geometry);
    }
    if (color) {
      auto &cc = registry.assign<Color>(entity);
      *&cc = *color;
    }
    if (texture_name) {
      auto &tc = registry.assign<TextureRenderable>(entity);
      auto texture_o = ttable.find(*texture_name);
      assert(texture_o);
      tc.texture_info = *texture_o;
    }

    if (pointlight_o) {
      auto &light_component = registry.assign<PointLight>(entity);
      light_component.light.diffuse = Color{*pointlight_o};
    }
    if (receives_light_o) {
      // TODO: fill in fields
      registry.assign<Material>(entity);
    }
    return entity;
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

stlw::result<AssetPair, std::string>
load_assets(stlw::Logger &logger, entt::DefaultRegistry &registry)
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

  std::cerr << "loading textures ...\n";
  auto texture_table = load_textures(logger, area_config);
  std::cerr << "loading entities ...\n";
  auto entities = load_entities(logger, area_config, texture_table, registry);

  std::cerr << "loading lights ...\n";
  auto const directional_light_diffuse = Color{get_vec3_or_abort(area_config, "directional_light_diffuse")};
  auto const directional_light_specular = Color{get_vec3_or_abort(area_config, "directional_light_specular")};
  auto const directional_light_direction = get_vec3_or_abort(area_config, "directional_light_direction");

  Light light{directional_light_diffuse, directional_light_specular};
  DirectionalLight dlight{MOVE(light), directional_light_direction};
  GlobalLight glight{MOVE(dlight)};

  auto const bg_color = Color{get_vec3_or_abort(area_config, "background")};
  auto const camera_spherical_coords_o = get_vec3(area_config, "camera_spherical_coords");
  auto const camera_spherical_coords = camera_spherical_coords_o ? *camera_spherical_coords_o : glm::zero<glm::vec3>();
  Assets assets{MOVE(meshes), MOVE(entities), MOVE(texture_table), MOVE(glight),
    bg_color, camera_spherical_coords};
  std::cerr << "yielding assets\n";
  return std::make_pair(MOVE(assets), MOVE(shader_programs));
}

} // ns boomhs
