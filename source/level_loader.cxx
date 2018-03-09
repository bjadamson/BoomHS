#include <boomhs/level_loader.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/components.hpp>
#include <boomhs/obj.hpp>

#include <stlw/result.hpp>

#include <extlibs/cpptoml.hpp>
#include <boost/algorithm/string/predicate.hpp>

// Test not needing

using namespace boomhs;
using namespace opengl;

using CppTableArray = std::shared_ptr<cpptoml::table_array>;
using CppTable = std::shared_ptr<cpptoml::table>;

#define TRY_OPTION_GENERAL_EVAL(VAR_NAME, V, expr)                                                 \
  auto V{expr};                                                                                    \
  if (!V) {                                                                                        \
    return cpptoml::option<opengl::AttributePointerInfo>{};                                        \
  }                                                                                                \
  VAR_NAME{MOVE(V)};

#define TRY_OPTION_CONCAT(VAR_NAME, TO_CONCAT, expr)                                               \
  TRY_OPTION_GENERAL_EVAL(VAR_NAME, _TRY_OPTION_TEMPORARY_##TO_CONCAT, expr)

#define TRY_OPTION_EXPAND_VAR(VAR_NAME, to_concat, expr)                                           \
  TRY_OPTION_CONCAT(VAR_NAME, to_concat, expr)

// TRY_OPTION
#define TRY_OPTION(VAR_NAME, expr)                                                                 \
  TRY_OPTION_EXPAND_VAR(VAR_NAME, __COUNTER__, expr)

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
std::optional<T>
get_value(CppTable const& table, char const* name)
{
  auto cpptoml_option = table->get_as<T>(name);
  if (!cpptoml_option) {
    return std::nullopt;
  }
  // Move value out of cpptoml and into std::optional
  auto value = cpptoml_option.move_out();
  return std::make_optional(MOVE(value));
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

std::optional<GLsizei>
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

std::optional<opengl::Color>
get_color(CppTable const& table, char const* name)
{
  auto const load_colors = table->template get_array_of<double>(name);
  if (!load_colors) {
    return std::nullopt;
  }

  std::vector<double> const& c = *load_colors;
  opengl::Color color;
  color.set_r(c[0]);
  color.set_g(c[1]);
  color.set_b(c[2]);
  color.set_a(c[3]);
  return std::make_optional(color);
}

std::optional<glm::vec3>
get_vec3(CppTable const& table, char const* name)
{
  auto const load_data = table->template get_array_of<double>(name);
  if (!load_data) {
    return std::nullopt;
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

std::optional<float>
get_float(CppTable const& table, char const* name)
{
  auto const load_data = get_value<double>(table, name);
  if (!load_data) {
    return std::nullopt;
  }
  return std::make_optional(static_cast<float>(*load_data));
}

float
get_float_or_abort(CppTable const& table, char const* name)
{
  auto const float_data = get_float(table, name);
  if (!float_data) {
    std::abort();
  }
  return *float_data;
}

Result<ObjStore, LoadStatus>
load_objfiles(CppTableArray const& mesh_table)
{
  auto const load = [](auto const& table) -> Result<std::pair<std::string, ObjData>, LoadStatus> {
    auto const name = get_string_or_abort(table, "name");
    std::cerr << "Loading objfile '" << name << "'\n";

    auto const obj = "assets/" + name + ".obj";
    auto const mtl = "assets/" + name + ".mtl";

    ObjData objdata = TRY_MOVEOUT(load_objfile(obj.c_str(), mtl.c_str()));
    auto pair = std::make_pair(name, MOVE(objdata));
    return OK_MOVE(pair);
  };
  ObjStore store;
  for (auto const& table : *mesh_table) {
    auto pair = TRY_MOVEOUT(load(table));
    store.add_obj(pair.first, MOVE(pair.second));
  }
  return OK_MOVE(store);
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

    auto const load_2dtexture = [&](auto const format)
    {
      auto const filename = get_string_or_abort(resource, "filename");
      opengl::TextureFilenames texture_names{name, {filename}};
      auto ta = opengl::texture::allocate_texture(logger, texture_names.filenames[0], format);
      ttable.add_texture(MOVE(texture_names), MOVE(ta));
    };
    auto const load_3dtexture = [&](auto const format)
    {
      auto const front = get_string_or_abort(resource, "front");
      auto const right = get_string_or_abort(resource, "right");
      auto const back = get_string_or_abort(resource, "back");
      auto const left = get_string_or_abort(resource, "left");
      auto const top = get_string_or_abort(resource, "top");
      auto const bottom = get_string_or_abort(resource, "bottom");

      opengl::TextureFilenames texture_names{name, {front, right, back, left, top, bottom}};
      auto ta = opengl::texture::upload_3dcube_texture(logger, texture_names.filenames, format);
      ttable.add_texture(MOVE(texture_names), MOVE(ta));
    };

    if (type == "texture:3dcube-RGB") {
      load_3dtexture(GL_RGB);
    }
    else if (type == "texture:3dcube-RGBA") {
      load_3dtexture(GL_RGBA);
    }
    else if (type == "texture:2d-RGBA") {
      load_2dtexture(GL_RGBA);
    }
    else if (type == "texture:2d-RGB") {
      load_2dtexture(GL_RGB);
    }
    else {
      // TODO: implement more.
      std::cerr << "error, type is '" << type << "'\n";
      std::abort();
    }
  };

  auto const resource_table = get_table_array_or_abort(config, "resource");
  std::for_each(resource_table->begin(), resource_table->end(), load_texture);
  return ttable;
}

struct ColorMaterialInfo
{
  Color const color;
  Material const material;
};

std::optional<ColorMaterialInfo>
load_material_color(CppTable const& file)
{
  // clang-format off
  auto const color = MAKEOPT(get_vec3(file,   "color"));
  auto const ambient = MAKEOPT(get_vec3(file,   "ambient"));
  auto const diffuse = MAKEOPT(get_vec3(file,   "diffuse"));
  auto const specular = MAKEOPT(get_vec3(file,   "specular"));
  auto const shininess = MAKEOPT(get_float(file,  "shininess"));
  // clang-format on

  Material material{ambient, diffuse, specular, shininess};
  ColorMaterialInfo const cmi{Color{color}, MOVE(material)};
  return std::make_optional(cmi);
}

auto
load_material_color_orabort(CppTable const& file)
{
  auto optional = load_material_color(file);
  if (!optional) {
    std::abort();
  }
  return *optional;
}

void
load_entities(stlw::Logger &logger, CppTable const& config, TextureTable const& ttable,
    EntityRegistry &registry)
{
  auto const load_entity = [&registry, &ttable](auto const& file) {
    // clang-format off
    auto shader       = get_string_or_abort(file, "shader");
    auto geometry     = get_string_or_abort(file, "geometry");
    auto pos          = get_vec3_or_abort(file,   "position");
    auto scale_o      = get_vec3(file,            "scale");
    auto rotation_o   = get_vec3(file,            "rotation");
    auto color        = get_color(file,           "color");
    auto texture_name = get_string(file,          "texture");
    auto pointlight_o = get_vec3(file,            "pointlight");
    auto player       = get_string(file,          "player");
    auto is_visible   = get_bool(file,            "is_visible").value_or(true);
    bool is_skybox    = get_bool(file,            "skybox").value_or(false);
    bool random_junk  = get_bool(file,            "random_junk_from_file").value_or(false);
    // clang-format on

    // texture OR color fields, not both
    assert((!color && !texture_name) || (!color && texture_name) || (color && !texture_name));

    auto eid = registry.create();
    auto &transform = registry.assign<Transform>(eid);
    transform.translation = pos;

    if (scale_o) {
      transform.scale = *scale_o;
    }
    if (rotation_o) {
      // TODO: simplify
      glm::vec3 const rotation = *rotation_o;
      transform.rotate_degrees(rotation.x, opengl::X_UNIT_VECTOR);
      transform.rotate_degrees(rotation.y, opengl::Y_UNIT_VECTOR);
      transform.rotate_degrees(rotation.z, opengl::Z_UNIT_VECTOR);
    }

    auto &isv = registry.assign<IsVisible>(eid);
    isv.value = is_visible;

    auto &sn = registry.assign<ShaderName>(eid);
    sn.value = shader;

    if (random_junk) {
      registry.assign<JunkEntityFromFILE>(eid);
    }

    if (player) {
      registry.assign<PlayerData>(eid);
    }
    if (geometry == "cube") {
      registry.assign<CubeRenderable>(eid);
    }
    if (is_skybox) {
      registry.assign<IsSkybox>(eid);
    }
    else if (boost::starts_with(geometry, "mesh")) {
      auto const parse_meshname = [](auto const& field) {
        auto const len = ::strlen("mesh:");
        assert(0 < len);
        return field.substr(len, field.length() - len);
      };
      auto &meshc = registry.assign<MeshRenderable>(eid);
      meshc.name = parse_meshname(geometry);
    }
    if (color) {
      auto &cc = registry.assign<Color>(eid);
      *&cc = *color;
    }
    if (texture_name) {
      auto &tr = registry.assign<TextureRenderable>(eid);
      auto texture_o = ttable.find(*texture_name);
      assert(texture_o);
      tr.texture_info = *texture_o;
    }

    if (pointlight_o) {
      auto &light_component = registry.assign<PointLight>(eid);
      light_component.light.diffuse = Color{*pointlight_o};
    }

    // An object receives light, if it has ALL ambient/diffuse/specular fields
    auto const cm_optional = load_material_color(file);
    if (cm_optional) {
      auto const& cm = *cm_optional;
      registry.assign<Material>(eid) = cm.material;
    }
  };

  auto const entity_table = get_table_array(config, "entity");
  for (auto const& it : *entity_table) {
    load_entity(it);
  }
}

auto
load_tileinfos(stlw::Logger &logger, CppTable const& config, EntityRegistry &registry)
{
  auto const load_tile = [](auto const& file) {
    auto const tile  =     get_string_or_abort(file, "tile");
    auto const tiletype = tiletype_from_string(tile);
    auto const mesh_name = get_string_or_abort(file, "mesh");
    auto const vshader_name = get_string_or_abort(file, "vshader");

    auto const cm = load_material_color_orabort(file);
    auto const& material = cm.material;
    auto const& color = cm.color;
    return TileInfo{tiletype, mesh_name, vshader_name, color, material};
  };
  auto const tile_table = get_table_array(config, "tile");
  auto const& ttable = tile_table->as_table_array()->get();

  // Ensure we load data for everry tile
  assert(TileSharedInfoTable::SIZE == ttable.size());

  std::array<TileInfo, TileSharedInfoTable::SIZE> tinfos;
  FOR(i, ttable.size()) {
    auto const& it = ttable[i];
    auto tile = load_tile(it);
    tinfos[i] = MOVE(tile);
  }
  return TileSharedInfoTable{MOVE(tinfos)};
}

using LoadResult = Result<std::pair<std::string, opengl::ShaderProgram>, std::string>;
LoadResult
load_shader(stlw::Logger &logger, ParsedVertexAttributes &pvas, CppTable const& table)
{
  auto const name = get_string_or_abort(table, "name");
  auto const vertex = get_string_or_abort(table, "vertex");
  auto const fragment = get_string_or_abort(table, "fragment");
  auto const va_name = get_string_or_abort(table, "va");

  // TODO: ugly hack, maybe think about...
  auto va = pvas.get_copy_of_va(va_name);
  auto program = TRY_MOVEOUT(opengl::make_shader_program(logger, vertex, fragment, MOVE(va)));

  program.is_skybox = get_bool(table, "is_skybox").value_or(false);
  program.is_2d = get_bool(table, "is_2d").value_or(false);
  program.instance_count = get_sizei(table, "instance_count");

  return Ok(std::make_pair(name, MOVE(program)));
}

Result<opengl::ShaderPrograms, std::string>
load_shaders(stlw::Logger &logger, ParsedVertexAttributes &&pvas, CppTable const& config)
{
  auto const shaders_table = get_table_array_or_abort(config, "shaders");
  opengl::ShaderPrograms sps;
  for (auto const& shader_table : *shaders_table) {
    auto pair = TRY_MOVEOUT(load_shader(logger, pvas, shader_table));
    sps.add(pair.first, MOVE(pair.second));
  }
  return Ok(MOVE(sps));
}

auto
load_vas(CppTable const& config)
{
  auto vas_table_array = config->get_table_array("vas");
  assert(vas_table_array);

  auto const read_data = [&](auto const& table, char const* fieldname, size_t &index) {
    // THINKING EXPLAINED:
    //
    // If there isn't a field, bail early. However, if there IS a field, ensure it has the fields
    // we expect.
    TRY_OPTION(auto data_table, table->get_table(fieldname));
    auto const datatype_s = get_string_or_abort(data_table, "datatype");

    // TODO: FOR NOW, only support floats. Easy to implement rest
    assert("float" == datatype_s);
    auto const datatype = GL_FLOAT;
    auto const num = get_or_abort<int>(data_table, "num");

    auto const uint_index = static_cast<GLuint>(index);
    auto const attribute_type = attribute_type_from_string(fieldname);
    auto api = opengl::AttributePointerInfo{uint_index, datatype, attribute_type, num};

    ++index;
    return cpptoml::option<opengl::AttributePointerInfo>{MOVE(api)};
  };

  auto const add_next_found = [&read_data](auto &apis, auto const& table, char const* fieldname,
      size_t &index)
  {
    auto data_o = read_data(table, fieldname, index);
    bool const data_read = !!data_o;
    if (data_read) {
      auto data = MOVE(*data_o);
      apis.emplace_back(MOVE(data));
    }
  };

  ParsedVertexAttributes pvas;
  auto const fn = [&pvas, &add_next_found](auto const& table) {
    auto const name = get_string_or_abort(table, "name");

    size_t i = 0u;
    std::vector<opengl::AttributePointerInfo> apis;
    add_next_found(apis, table, "position", i);
    add_next_found(apis, table, "normal", i);
    add_next_found(apis, table, "color", i);
    add_next_found(apis, table, "uv", i);

    pvas.add(name, make_vertex_attribute(apis));
  };
  std::for_each((*vas_table_array).begin(), (*vas_table_array).end(), fn);
  return pvas;
}

} // ns anon

namespace boomhs
{

// This macro exists to reduce code duplication implementing the two different implementation of
// operator[].
#define SEARCH_FOR(type, begin, end)                                                               \
  auto const cmp = [&type](auto const& tinfo) {                                                    \
    return tinfo.type == type;                                                                     \
  };                                                                                               \
  auto const it = std::find_if(begin, end, cmp);                                                   \
  assert(it != end);                                                                               \
  return *it;

////////////////////////////////////////////////////////////////////////////////////////////////////
// TileSharedInfoTable
TileInfo const&
TileSharedInfoTable::operator[](TileType const type) const
{
  SEARCH_FOR(type, data_.cbegin(), data_.cend());
}

TileInfo&
TileSharedInfoTable::operator[](TileType const type)
{
  SEARCH_FOR(type, data_.begin(), data_.end());
}
#undef SEARCH_FOR

///////////////////////////////////////////////////////////////////////////////////////////////////
// LevelLoader
Result<LevelAssets, std::string>
LevelLoader::load_level(stlw::Logger &logger, EntityRegistry &registry, std::string const& filename)
{
  CppTable engine_config = cpptoml::parse_file("engine.toml");
  assert(engine_config);

  ParsedVertexAttributes pvas = load_vas(engine_config);
  auto sps = TRY_MOVEOUT(load_shaders(logger, MOVE(pvas), engine_config));

  CppTable area_config = cpptoml::parse_file("levels/" + filename);
  assert(area_config);

  auto const mesh_table = get_table_array_or_abort(area_config, "meshes");
  ObjStore objstore = TRY_MOVEOUT(load_objfiles(mesh_table).mapErrorMoveOut(loadstatus_to_string));

  std::cerr << "loading textures ...\n";
  auto texture_table = load_textures(logger, area_config);
  std::cerr << "loading entities ...\n";
  load_entities(logger, area_config, texture_table, registry);

  std::cerr << "loading tile materials ...\n";
  auto tile_table = load_tileinfos(logger, area_config, registry);

  std::cerr << "loading lights ...\n";
  auto const ambient = Color{get_vec3_or_abort(area_config, "ambient")};
  auto const directional_light_diffuse = Color{get_vec3_or_abort(area_config, "directional_light_diffuse")};
  auto const directional_light_specular = Color{get_vec3_or_abort(area_config, "directional_light_specular")};
  auto const directional_light_direction = get_vec3_or_abort(area_config, "directional_light_direction");

  Light light{directional_light_diffuse, directional_light_specular};
  DirectionalLight dlight{MOVE(light), directional_light_direction};
  GlobalLight glight{ambient, MOVE(dlight)};

  auto bg_color = Color{get_vec3_or_abort(area_config, "background")};
  std::cerr << "yielding assets\n";
  return Ok(LevelAssets{
    MOVE(glight),
    MOVE(bg_color),

    MOVE(tile_table),

    MOVE(objstore),
    MOVE(texture_table),
    MOVE(sps)
  });
}

} // ns boomhs
