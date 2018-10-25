#include <boomhs/billboard.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/material.hpp>
#include <boomhs/obj.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/water.hpp>

#include <common/algorithm.hpp>
#include <common/result.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <extlibs/cpptoml.hpp>

using namespace boomhs;
using namespace opengl;

using CppTableArray = std::shared_ptr<cpptoml::table_array>;
using CppTable      = std::shared_ptr<cpptoml::table>;

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
#define TRY_OPTION(VAR_NAME, expr) TRY_OPTION_EXPAND_VAR(VAR_NAME, __COUNTER__, expr)

namespace
{

CppTable
get_table(CppTable const& table, char const* name)
{
  return table->get_table_qualified(name);
}

CppTable
get_table_or_abort(CppTable const& table, char const* name)
{

  auto table_o = get_table(table, name);
  if (!table_o) {
    std::abort();
  }
  return table_o;
}

CppTableArray
get_table_array(CppTable const& table, char const* name)
{
  return table->get_table_array(name);
}

CppTableArray
get_table_array_or_abort(CppTable const& table, char const* name)
{
  auto table_o = get_table_array(table, name);
  if (!table_o) {
    std::abort();
  }
  return table_o;
}

template <typename T>
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

template <typename T>
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

std::optional<Color>
get_color(CppTable const& table, char const* name)
{
  auto const load_colors = table->template get_array_of<double>(name);
  if (!load_colors) {
    return std::nullopt;
  }

  std::vector<double> const& c = *load_colors;
  assert(4 == c.size() || 3 == c.size());

  auto const alpha = (3 == c.size()) ? 1.0f : c[3];
  auto color       = color::make_rgba(c[0], c[1], c[2], alpha);
  return std::make_optional(color);
}

Color
get_color_or_abort(CppTable const& table, char const* name)
{
  auto const c = get_color(table, name);
  if (c) {
    return *c;
  }
  std::abort();
}

auto const&
get_first_and_only_entry(CppTable const& table, char const* name)
{
  auto const ta = get_table_array(table, name);

  // Ensure the value retrieved from the table is itself a table, and get a reference to it.
  assert(ta->is_table_array());
  auto const& table_array = ta->get();

  // Confirm there is only one entry in the table array.
  if (1 != table_array.size()) {
    std::abort();
  }

  // Return a reference to the first (and only) entry in the table array.
  return table_array.front();
}

std::optional<glm::vec2>
get_vec2(CppTable const& table, char const* name)
{
  auto const load_data = table->template get_array_of<double>(name);
  if (!load_data) {
    return std::nullopt;
  }
  auto const& ld = *load_data;
  return glm::vec2{ld[0], ld[1]};
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

std::optional<int>
get_int(CppTable const& table, char const* name)
{
  auto const load_data = get_value<int>(table, name);
  if (!load_data) {
    return std::nullopt;
  }
  return std::make_optional(*load_data);
}

int
get_int_or_abort(CppTable const& table, char const* name)
{
  auto const data = get_int(table, name);
  if (!data) {
    std::abort();
  }
  return *data;
}

std::optional<unsigned int>
get_unsignedint(CppTable const& table, char const* name)
{
  auto const load_data = get_value<unsigned int>(table, name);
  if (!load_data) {
    return std::nullopt;
  }
  return std::make_optional(*load_data);
}

unsigned int
get_unsignedint_or_abort(CppTable const& table, char const* name)
{
  auto const data = get_unsignedint(table, name);
  if (!data) {
    std::abort();
  }
  return *data;
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
load_objfiles(common::Logger& logger, CppTableArray const& mesh_table)
{
  auto const load = [&](auto const& table) -> Result<std::pair<std::string, ObjData>, LoadStatus> {
    auto const path = get_string_or_abort(table, "path");
    auto const name = get_string_or_abort(table, "name");
    LOG_TRACE_SPRINTF("Loading objfile name: '%s' path: '%s'", name, path);

    auto const objname = name + ".obj";
    ObjData    objdata = TRY_MOVEOUT(load_objfile(logger, path, objname));
    auto       pair    = std::make_pair(name, MOVE(objdata));
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

  void add(std::string const& name, opengl::VertexAttribute&& va)
  {
    auto pair = std::make_pair(name, MOVE(va));
    pair_.emplace_back(MOVE(pair));
  }

  opengl::VertexAttribute get_copy_of_va(std::string const& va_name)
  {
    auto const cmp     = [&va_name](auto const& it) { return it.first == va_name; };
    auto       find_it = std::find_if(pair_.cbegin(), pair_.cend(), cmp);

    // TODO: for now assume we always find requested VA.
    assert(find_it != pair_.cend());
    return find_it->second.clone();
  }
};

Result<opengl::TextureTable, std::string>
load_textures(common::Logger& logger, CppTable const& table)
{
  opengl::TextureTable ttable;
  auto const           load_texture = [&logger,
                             &ttable](auto const& resource) -> Result<common::none_t, std::string> {
    auto const name = get_string_or_abort(resource, "name");
    auto const type = get_string_or_abort(resource, "type");

    auto const  wrap_s    = get_string(resource, "wrap").value_or("clamp");
    GLint const wrap_mode = texture::wrap_mode_from_string(wrap_s.c_str());

    auto const uv_max = get_float(resource, "uvs").value_or(1.0f);

    unsigned int const texture_unit = get_unsignedint(resource, "texture_unit").value_or(0);

    TextureInfo ti;
    ti.wrap   = wrap_mode;
    ti.uv_max = uv_max;

    auto const load_2dtexture = [&](GLenum const format) -> Result<common::none_t, std::string> {
      auto const               filename = get_string_or_abort(resource, "filename");
      opengl::TextureFilenames texture_names{name, {filename}};

      ti.format = format;
      Texture t = TRY_MOVEOUT(
          opengl::texture::upload_2d_texture(logger, texture_names.filenames[0], MOVE(ti)));

      ttable.add_texture(MOVE(texture_names), MOVE(t));
      return OK_NONE;
    };
    auto const load_3dtexture = [&](GLenum const format) -> Result<common::none_t, std::string> {
      auto const front  = get_string_or_abort(resource, "front");
      auto const right  = get_string_or_abort(resource, "right");
      auto const back   = get_string_or_abort(resource, "back");
      auto const left   = get_string_or_abort(resource, "left");
      auto const top    = get_string_or_abort(resource, "top");
      auto const bottom = get_string_or_abort(resource, "bottom");

      opengl::TextureFilenames texture_names{name, {front, right, back, left, top, bottom}};
      ti.format = format;

      Texture t = TRY_MOVEOUT(
          opengl::texture::upload_3dcube_texture(logger, texture_names.filenames, MOVE(ti)));

      ttable.add_texture(MOVE(texture_names), MOVE(t));
      return OK_NONE;
    };

    if (type == "texture:3dcube-RGB") {
      TRY_MOVEOUT(load_3dtexture(GL_RGB));
    }
    else if (type == "texture:3dcube-RGBA") {
      TRY_MOVEOUT(load_3dtexture(GL_RGBA));
    }
    else if (type == "texture:2d-RGBA") {
      TRY_MOVEOUT(load_2dtexture(GL_RGBA));
    }
    else if (type == "texture:2d-RGB") {
      TRY_MOVEOUT(load_2dtexture(GL_RGB));
    }
    else {
      // TODO: implement more.
      LOG_ERROR_SPRINTF("error, type is: %s", type);
      std::abort();
    }

    return OK_NONE;
  };

  auto const resource_table = get_table_array_or_abort(table, "resource");
  std::for_each(resource_table->begin(), resource_table->end(), load_texture);
  return OK_MOVE(ttable);
}

auto
load_fog(CppTable const& file)
{
  auto const& data     = get_first_and_only_entry(file, "fog");
  auto const  density  = get_float_or_abort(data, "density");
  auto const  gradient = get_float_or_abort(data, "gradient");
  auto const  color    = get_color_or_abort(data, "color");

  return Fog{density, gradient, color};
}

std::optional<NameMaterial>
load_material(CppTable const& file)
{
  // clang-format off
  auto const name      = get_string_or_abort(file, "name");
  auto const ambient   = MAKEOPT(get_vec3(file,  "ambient"));
  auto const diffuse   = MAKEOPT(get_vec3(file,  "diffuse"));
  auto const specular  = MAKEOPT(get_vec3(file,  "specular"));
  auto const shininess = MAKEOPT(get_float(file, "shininess"));
  // clang-format on

  Material const material{ambient, diffuse, specular, shininess};
  return std::make_optional(NameMaterial{name, material});
}

auto
load_material_or_abort(CppTable const& file)
{
  auto optional = load_material(file);
  if (!optional) {
    std::abort();
  }
  return *optional;
}

auto
load_attenuation(CppTable const& file)
{
  auto const name      = get_string_or_abort(file, "name");
  auto const constant  = get_float(file, "constant").value_or(0);
  auto const linear    = get_float(file, "linear").value_or(0);
  auto const quadratic = get_float(file, "quadratic").value_or(0);

  return NameAttenuation{name, Attenuation{constant, linear, quadratic}};
}

auto
load_attenuations(common::Logger& logger, CppTable const& table)
{
  auto const                   table_array = get_table_array(table, "attenuation");
  std::vector<NameAttenuation> result;
  for (auto const& it : *table_array) {
    result.emplace_back(load_attenuation(it));
  }
  return result;
}

auto
load_materials(common::Logger& logger, CppTable const& table)
{
  auto const table_array = get_table_array(table, "material");

  MaterialTable material_table;
  for (auto const& it : *table_array) {
    auto material = load_material_or_abort(it);
    material_table.add(MOVE(material));
  }
  return material_table;
}

void
load_entities(common::Logger& logger, CppTable const& level_table, LevelAssets& assets,
              EntityRegistry& registry)
{
  auto&       ttable       = assets.texture_table;
  auto const& mtable       = assets.material_table;
  auto const& attenuations = assets.attenuations;

  auto const load_entity = [&](auto const& file) {
    // clang-format off
    auto const name          = get_string(file,          "name").value_or("FromFileUnnamed");
    auto const shader        = get_string_or_abort(file, "shader");
    auto const geometry      = get_string_or_abort(file, "geometry");
    auto const pos           = get_vec3_or_abort(file,   "position");
    auto const scale_o       = get_vec3(file,            "scale");
    auto const rotation_o    = get_vec3(file,            "rotation");
    auto const color         = get_color(file,           "color");
    auto const material_o    = get_string(file,          "material");
    auto const texture_name  = get_string(file,          "texture");
    auto const is_hidden     = get_bool(file,            "hidden").value_or(false);
    bool const random_junk   = get_bool(file,            "random_junk_from_file").value_or(false);

    // sub-tables or "inner"-tables
    auto const orbital_o       = get_table(file, "orbital-body");
    auto const pointlight_o    = get_table(file, "pointlight");
    // clang-format on

    // texture OR color fields, not both
    assert((!color && !texture_name) || (!color && texture_name) || (color && !texture_name));

    auto eid = registry.create();
    registry.assign<Name>(eid, name);

    auto& transform       = registry.assign<Transform>(eid);
    transform.translation = pos;

    registry.assign<IsRenderable>(eid, is_hidden);
    registry.assign<ShaderName>(eid, shader);

    if (scale_o) {
      transform.scale = *scale_o;
    }
    if (rotation_o) {
      transform.rotate_xyz_degrees(*rotation_o);
    }

    if (random_junk) {
      registry.assign<JunkEntityFromFILE>(eid);
    }

    if (orbital_o) {
      auto const x = get_float_or_abort(orbital_o, "x");
      auto const y = get_float_or_abort(orbital_o, "y");
      auto const z = get_float_or_abort(orbital_o, "z");

      auto const radius = glm::vec3{x, y, z};
      auto const offset = get_float(orbital_o, "offset").value_or(0.0);
      registry.assign<OrbitalBody>(eid, radius, offset);
    }

    if (geometry == "cube") {
      auto&      cr              = registry.assign<CubeRenderable>(eid);
      auto const cube_vertices_o = get_table_or_abort(file, "cube_vertices");
      cr.min                     = get_vec3(cube_vertices_o, "min").value_or(glm::vec3{1.0f});
      cr.max                     = get_vec3(cube_vertices_o, "max").value_or(glm::vec3{1.0f});
    }
    else if (boost::starts_with(geometry, "mesh")) {
      auto const parse_meshname = [](auto const& field) {
        auto const len = ::strlen("mesh:");
        assert(0 < len);
        return field.substr(len, field.length() - len);
      };
      auto  mesh_name = parse_meshname(geometry);
      auto& meshc     = registry.assign<MeshRenderable>(eid, MOVE(mesh_name));
    }
    else if (boost::starts_with(geometry, "billboard")) {
      auto const parse_billboard = [](auto const& field) {
        auto const len = ::strlen("billboard:");
        assert(0 < len);
        auto const str = field.substr(len, field.length() - len);
        return Billboard::from_string(str);
      };
      auto& billboard = registry.assign<BillboardRenderable>(eid);
      billboard.value = parse_billboard(geometry);
    }
    if (color) {
      registry.assign<Color>(eid, *color);
    }
    if (texture_name) {
      LOG_DEBUG_SPRINTF("Looking up texture %s", *texture_name);
      auto& tr        = registry.assign<TextureRenderable>(eid);
      auto  texture_o = ttable.find(*texture_name);
      assert(texture_o);
      tr.texture_info = &*texture_o;
    }

    if (pointlight_o) {
      auto const attenuation = get_string_or_abort(pointlight_o, "attenuation");

      auto const cmp = [&](NameAttenuation const& na) { return na.name == attenuation; };
      auto const it  = std::find_if(attenuations.cbegin(), attenuations.cend(), cmp);
      assert(it != attenuations.cend());

      auto& pl       = registry.assign<PointLight>(eid);
      pl.attenuation = it->value;

      auto& light    = pl.light;
      // TODO: instead of converting the vec4 to vec3, just read it in as a vec3.
      light.diffuse  = get_color_or_abort(pointlight_o, "diffuse").rgb();
      light.specular = get_color_or_abort(pointlight_o, "specular").rgb();
    }

    auto& obj_store = assets.obj_store;
    if (common::cstrcmp(name.c_str(), "TreeLowpoly")) {
      auto& obj = obj_store.get(logger, registry.get<MeshRenderable>(eid).name);
      auto& tc  = registry.assign<TreeComponent>(eid, obj);
      tc.add_color(TreeColorType::Leaf, LOC4::GREEN);
      tc.add_color(TreeColorType::Leaf, LOC4::PINK);
      tc.add_color(TreeColorType::Trunk, LOC4::BROWN);
    }
    if (common::cstrcmp(name.c_str(), "Tree2")) {
      auto& obj = obj_store.get(logger, registry.get<MeshRenderable>(eid).name);
      auto& tc  = registry.assign<TreeComponent>(eid, obj);
      tc.add_color(TreeColorType::Leaf, LOC4::YELLOW);
      tc.add_color(TreeColorType::Stem, LOC4::RED);
      tc.add_color(TreeColorType::Stem, LOC4::BLUE);
      tc.add_color(TreeColorType::Trunk, LOC4::GREEN);
    }

    // An object receives light, if it has ALL ambient/diffuse/specular fields
    if (material_o) {
      auto const  material_name      = *material_o;
      auto const& material           = mtable.find(material_name);
      registry.assign<Material>(eid) = material;
    }
  };

  auto const entity_table = get_table_array(level_table, "entity");
  for (auto const& it : *entity_table) {
    load_entity(it);
  }
}

using LoadResult = Result<std::pair<std::string, opengl::ShaderProgram>, std::string>;
LoadResult
load_shader(common::Logger& logger, ParsedVertexAttributes& pvas, CppTable const& table)
{
  auto const name     = get_string_or_abort(table, "name");
  auto const vertex   = get_string_or_abort(table, "vertex");
  auto const fragment = get_string_or_abort(table, "fragment");
  auto const va_name  = get_string_or_abort(table, "va");

  // TODO: ugly hack, maybe think about...
  auto va      = pvas.get_copy_of_va(va_name);
  auto program = TRY_MOVEOUT(opengl::make_shader_program(logger, vertex, fragment, MOVE(va)));

  program.is_2d          = get_bool(table, "is_2d").value_or(false);
  program.instance_count = get_sizei(table, "instance_count");

  return Ok(std::make_pair(name, MOVE(program)));
}

Result<opengl::ShaderPrograms, std::string>
load_shaders(common::Logger& logger, ParsedVertexAttributes&& pvas, CppTable const& table)
{
  auto const             shaders_table = get_table_array_or_abort(table, "shaders");
  opengl::ShaderPrograms sps;
  for (auto const& shader_table : *shaders_table) {
    auto pair = TRY_MOVEOUT(load_shader(logger, pvas, shader_table));
    sps.add(pair.first, MOVE(pair.second));
  }
  return Ok(MOVE(sps));
}

struct TerrainData
{
  std::string mesh_name;
  int         terrain_index;
  int         water_index;
};

auto
load_vas(CppTable const& table)
{
  auto const table_array = table->get_table_array("vas");
  assert(table_array);

  auto const read_data = [&](auto const& table, char const* fieldname, size_t& index) {
    // THINKING EXPLAINED:
    //
    // If there isn't a field, bail early. However, if there IS a field, ensure it has the fields
    // we expect.
    TRY_OPTION(auto data_table, table->get_table(fieldname));
    auto const datatype_s = get_string_or_abort(data_table, "datatype");

    // TODO: FOR NOW, only support floats. Easy to implement rest
    assert("float" == datatype_s);
    auto const datatype = GL_FLOAT;
    auto const num      = get_or_abort<int>(data_table, "num");

    auto const                   uint_index     = static_cast<GLuint>(index);
    auto const                   attribute_type = attribute_type_from_string(fieldname);
    opengl::AttributePointerInfo api{uint_index, datatype, attribute_type, num};

    ++index;
    return cpptoml::option<opengl::AttributePointerInfo>{MOVE(api)};
  };

  auto const add_next_found = [&read_data](auto& apis, auto const& table, char const* fieldname,
                                           size_t& index) {
    auto       data_o    = read_data(table, fieldname, index);
    bool const data_read = !!data_o;
    if (data_read) {
      auto data = MOVE(*data_o);
      apis.emplace_back(MOVE(data));
    }
  };

  ParsedVertexAttributes pvas;
  auto const             fn = [&pvas, &add_next_found](auto const& table) {
    auto const name = get_string_or_abort(table, "name");

    size_t                                    i = 0u;
    std::vector<opengl::AttributePointerInfo> apis;
    add_next_found(apis, table, "position", i);
    add_next_found(apis, table, "normal", i);
    add_next_found(apis, table, "color", i);
    add_next_found(apis, table, "uv", i);

    pvas.add(name, make_vertex_attribute(apis));
  };
  std::for_each((*table_array).begin(), (*table_array).end(), fn);
  return pvas;
}

auto
load_global_lighting(CppTable const& table)
{
  auto const global_lighting = get_first_and_only_entry(table, "global-lighting");
  auto const ambient         = get_color_or_abort(global_lighting, "ambient").rgb();

  auto const directinal_table = get_table_or_abort(global_lighting, "directional");
  auto const diffuse          = get_color_or_abort(directinal_table, "diffuse").rgb();
  auto const specular         = get_color_or_abort(directinal_table, "specular").rgb();
  auto const direction        = get_vec3_or_abort(directinal_table, "direction");

  Light            light{diffuse, specular};
  DirectionalLight dlight{MOVE(light), direction};
  return GlobalLight{ambient, MOVE(dlight)};
}

} // namespace

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// LevelLoader
Result<LevelAssets, std::string>
LevelLoader::load_level(common::Logger& logger, EntityRegistry& registry,
                        std::string const& filename)
{
  CppTable engine_table = cpptoml::parse_file("engine.toml");
  assert(engine_table);

  ParsedVertexAttributes pvas = load_vas(engine_table);
  auto                   sps  = TRY_MOVEOUT(load_shaders(logger, MOVE(pvas), engine_table));

  CppTable resource_table = cpptoml::parse_file("levels/resources.toml");
  assert(resource_table);

  auto const mesh_table = get_table_array_or_abort(resource_table, "meshes");
  ObjStore   objstore =
      TRY_MOVEOUT(load_objfiles(logger, mesh_table).mapErrorMoveOut(loadstatus_to_string));

  LOG_TRACE("loading level data begin ...");
  LOG_TRACE("textures ...");
  auto texture_table = TRY_MOVEOUT(load_textures(logger, resource_table));

  LOG_TRACE("materials ...");
  auto material_table = load_materials(logger, resource_table);

  LOG_TRACE("attenuations ...");
  auto attenuations = load_attenuations(logger, resource_table);

  CppTable level_table = cpptoml::parse_file("levels/" + filename);
  assert(level_table);

  LOG_TRACE("global lighting ...");
  auto glight = load_global_lighting(level_table);

  LOG_TRACE("global fog ...");
  auto fog = load_fog(level_table);

  LOG_TRACE("loading level finished successfully!");
  LevelAssets assets{MOVE(glight),       MOVE(fog),           MOVE(material_table),
                     MOVE(attenuations),

                     MOVE(objstore),     MOVE(texture_table), MOVE(sps)};
  load_entities(logger, level_table, assets, registry);
  return OK_MOVE(assets);
}

} // namespace boomhs
