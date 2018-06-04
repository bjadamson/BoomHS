#include <boomhs/billboard.hpp>
#include <boomhs/components.hpp>
#include <boomhs/entity.hpp>
#include <boomhs/level_loader.hpp>
#include <boomhs/obj.hpp>
#include <boomhs/orbital_body.hpp>
#include <boomhs/player.hpp>
#include <boomhs/skybox.hpp>
#include <boomhs/tree.hpp>
#include <boomhs/water.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/result.hpp>

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

std::optional<opengl::Color>
get_color(CppTable const& table, char const* name)
{
  auto const load_colors = table->template get_array_of<double>(name);
  if (!load_colors) {
    return std::nullopt;
  }

  std::vector<double> const& c = *load_colors;
  assert(4 == c.size());

  opengl::Color color;
  color.set_r(c[0]);
  color.set_g(c[1]);
  color.set_b(c[2]);
  color.set_a(c[3]);
  return std::make_optional(color);
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
load_objfiles(stlw::Logger& logger, CppTableArray const& mesh_table)
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
    return find_it->second;
  }
};

Result<opengl::TextureTable, std::string>
load_textures(stlw::Logger& logger, CppTable const& table)
{
  opengl::TextureTable ttable;
  auto const           load_texture = [&logger,
                             &ttable](auto const& resource) -> Result<stlw::none_t, std::string> {
    auto const name = get_string_or_abort(resource, "name");
    auto const type = get_string_or_abort(resource, "type");

    auto const load_2dtexture = [&](auto const format) -> Result<stlw::none_t, std::string> {
      auto const               filename = get_string_or_abort(resource, "filename");
      opengl::TextureFilenames texture_names{name, {filename}};

      auto const  wrap_s = get_string(resource, "wrap").value_or("clamp");
      GLint const wrap   = texture::wrap_mode_from_string(wrap_s.c_str());

      auto const uv_max = get_float(resource, "uvs").value_or(1.0f);

      unsigned int const texture_unit = get_unsignedint(resource, "texture_unit").value_or(0);

      opengl::texture::TextureAllocationArgs const taa{format, uv_max, texture_unit};

      Texture t =
          TRY_MOVEOUT(opengl::texture::allocate_texture(logger, texture_names.filenames[0], taa));

      glActiveTexture(GL_TEXTURE0 + texture_unit);
      ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

      t->while_bound(logger, [&]() {
        t->set_fieldi(GL_TEXTURE_WRAP_S, wrap);
        t->set_fieldi(GL_TEXTURE_WRAP_T, wrap);

        // Set texture filtering parameters
        t->set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        t->set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      });

      ttable.add_texture(MOVE(texture_names), MOVE(t));
      return OK_NONE;
    };
    auto const load_3dtexture = [&](auto const format) -> Result<stlw::none_t, std::string> {
      auto const front  = get_string_or_abort(resource, "front");
      auto const right  = get_string_or_abort(resource, "right");
      auto const back   = get_string_or_abort(resource, "back");
      auto const left   = get_string_or_abort(resource, "left");
      auto const top    = get_string_or_abort(resource, "top");
      auto const bottom = get_string_or_abort(resource, "bottom");

      opengl::TextureFilenames texture_names{name, {front, right, back, left, top, bottom}};
      Texture                  t = TRY_MOVEOUT(
          opengl::texture::upload_3dcube_texture(logger, texture_names.filenames, format));

      t->while_bound(logger, [&]() {
        t->set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        t->set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        t->set_fieldi(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        t->set_fieldi(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        t->set_fieldi(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      });
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

std::optional<Color>
load_color(CppTable const& file, char const* name)
{
  return Color{MAKEOPT(get_vec3(file, name))};
}

Color
load_color_or_abort(CppTable const& file, char const* name)
{
  auto optional = load_color(file, name);
  if (!optional) {
    std::abort();
  }
  return *optional;
}

auto
load_fog(CppTable const& file)
{
  auto const table = get_table_array(file, "fog");

  // This is required in order to downcast table to a sized table array.
  assert(table->is_table_array());
  auto const& table_array = table->get();

  // For now, assume exactly one fog entry.
  assert(1 == table_array.size());

  auto const& data     = table_array.front();
  auto const  density  = get_float_or_abort(data, "density");
  auto const  gradient = get_float_or_abort(data, "gradient");
  auto const  color    = load_color_or_abort(data, "color");

  return Fog{density, gradient, color};
}

struct NameMaterial
{
  std::string const name;
  Material const    material;
};

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

struct NameAttenuation
{
  std::string const name;
  Attenuation const attenuation;
};

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
load_attenuations(stlw::Logger& logger, CppTable const& table, EntityRegistry& registry)
{
  auto const                   table_array = get_table_array(table, "attenuation");
  std::vector<NameAttenuation> result;
  for (auto const& it : *table_array) {
    result.emplace_back(load_attenuation(it));
  }
  return result;
}

auto
load_materials(stlw::Logger& logger, CppTable const& table, EntityRegistry& registry)
{
  auto const                table_array = get_table_array(table, "material");
  std::vector<NameMaterial> result;
  for (auto const& it : *table_array) {
    auto const material = load_material_or_abort(it);
    result.emplace_back(material);
  }
  return result;
}

struct NamePointlight
{
  std::string const name;
  PointLight const  pointlight;
};

auto
load_pointlight(CppTable const& file, std::vector<NameAttenuation> const& attenuations)
{
  // clang-format off
  auto const name          = get_string_or_abort(file, "name");
  auto const attenuation_o = get_string(file,          "attenuation");
  auto const diffuse       = get_vec3_or_abort(file,   "diffuse");
  auto const specular      = get_vec3_or_abort(file,   "specular");
  // clang-format on

  PointLight pl;
  pl.light = Light{Color{diffuse}, Color{specular}};

  if (attenuation_o) {
    auto const name = *attenuation_o;
    auto const cmp  = [&name](NameAttenuation const& na) { return na.name == name; };
    auto const it   = std::find_if(attenuations.cbegin(), attenuations.cend(), cmp);
    assert(it != attenuations.cend());
    pl.attenuation = it->attenuation;
  }
  return NamePointlight{name, pl};
}

auto
load_pointlights(stlw::Logger& logger, CppTable const& table,
                 std::vector<NameAttenuation> const& attenuations)
{
  auto const                  table_array = get_table_array(table, "pointlight");
  std::vector<NamePointlight> result;
  for (auto const& it : *table_array) {
    auto const pointlight = load_pointlight(it, attenuations);
    result.emplace_back(pointlight);
  }
  return result;
}

auto
load_orbital_bodies(stlw::Logger& logger, CppTable const& table, EntityRegistry& registry)
{
  auto const load = [](auto const& file) {
    // clang-format off
    auto const name        = get_string_or_abort(file, "name");
    float const x_radius   = get_float_or_abort(file,  "x_radius");
    float const y_radius   = get_float_or_abort(file,  "y_radius");
    float const z_radius   = get_float_or_abort(file,  "z_radius");
    float const offset     = get_float(file,           "offset").value_or(0.0f);
    return OrbitalBody{name, x_radius, y_radius, z_radius, offset};
    // clang-format on
  };
  auto const orbital_body_table = get_table_array(table, "orbital-body");

  std::vector<OrbitalBody> orbitals;
  for (auto const& it : *orbital_body_table) {
    orbitals.emplace_back(load(it));
  }
  return orbitals;
}

void
load_entities(stlw::Logger& logger, CppTable const& table,
              std::vector<OrbitalBody> const& orbital_bodies, TextureTable& ttable,
              std::vector<NameMaterial> const&   materials,
              std::vector<NamePointlight> const& pointlights, EntityRegistry& registry)
{
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
    auto const pointlight_o  = get_string(file,          "pointlight");
    auto const player        = get_string(file,          "player");
    auto const is_visible    = get_bool(file,            "is_visible").value_or(true);
    bool const random_junk   = get_bool(file,            "random_junk_from_file").value_or(false);
    bool const is_water      = get_bool(file,            "water").value_or(false);
    auto const orbital_o     = get_string(file,          "orbital-body");
    // clang-format on

    // texture OR color fields, not both
    assert((!color && !texture_name) || (!color && texture_name) || (color && !texture_name));

    auto eid                         = registry.create();
    registry.assign<Name>(eid).value = name;

    auto& transform       = registry.assign<Transform>(eid);
    transform.translation = pos;

    auto& isv = registry.assign<IsVisible>(eid);
    isv.value = is_visible;

    auto& sn = registry.assign<ShaderName>(eid);
    sn.value = shader;

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

    if (random_junk) {
      registry.assign<JunkEntityFromFILE>(eid);
    }

    if (orbital_o) {
      auto&      orbital = registry.assign<OrbitalBody>(eid);
      auto const cmp     = [&orbital_o](auto const& orbital) { return orbital.name == *orbital_o; };
      auto const it      = std::find_if(orbital_bodies.cbegin(), orbital_bodies.cend(), cmp);
      assert(it != orbital_bodies.cend());
      orbital = *it;
    }

    if (player) {
      registry.assign<PlayerData>(eid);
    }
    if (geometry == "cube") {
      auto& cr = registry.assign<CubeRenderable>(eid);
      cr.min   = glm::vec3{-0.5f};
      cr.max   = glm::vec3{0.5f};
    }
    else if (boost::starts_with(geometry, "mesh")) {
      auto const parse_meshname = [](auto const& field) {
        auto const len = ::strlen("mesh:");
        assert(0 < len);
        return field.substr(len, field.length() - len);
      };
      auto& meshc = registry.assign<MeshRenderable>(eid);
      meshc.name  = parse_meshname(geometry);
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
      auto& cc = registry.assign<Color>(eid);
      *&cc     = *color;
    }
    if (texture_name) {
      auto& tr        = registry.assign<TextureRenderable>(eid);
      auto  texture_o = ttable.find(*texture_name);
      assert(texture_o);
      tr.texture_info = &*texture_o;
    }

    if (pointlight_o) {
      auto const cmp = [&pointlight_o](NamePointlight const& np) {
        return np.name == *pointlight_o;
      };
      auto const it = std::find_if(pointlights.cbegin(), pointlights.cend(), cmp);
      assert(it != pointlights.cend());
      registry.assign<PointLight>(eid) = it->pointlight;
    }

    if (name == "tree") {
      registry.assign<TreeComponent>(eid);
    }

    // An object receives light, if it has ALL ambient/diffuse/specular fields
    if (material_o) {
      auto const material_name = *material_o;
      auto const cmp           = [&material_name](NameMaterial const& nm) {
        return nm.name == material_name;
      };
      auto const it = std::find_if(materials.cbegin(), materials.cend(), cmp);
      assert(it != materials.cend());
      registry.assign<Material>(eid) = it->material;
    }

    if (is_water) {
      registry.assign<WaterTileThing>(eid);
    }
  };

  auto const entity_table = get_table_array(table, "entity");
  for (auto const& it : *entity_table) {
    load_entity(it);
  }
}

auto
load_tileinfos(stlw::Logger& logger, CppTable const& table,
               std::vector<NameMaterial> const& materials, EntityRegistry& registry)
{
  auto const load_tile = [&materials](auto const& file) {
    auto const tile          = get_string_or_abort(file, "tile");
    auto const tiletype      = tiletype_from_string(tile);
    auto const mesh_name     = get_string_or_abort(file, "mesh");
    auto const vshader_name  = get_string_or_abort(file, "vshader");
    auto const material_name = get_string_or_abort(file, "material");

    auto const cmp = [&material_name](NameMaterial const& nm) { return nm.name == material_name; };
    auto const it  = std::find_if(materials.cbegin(), materials.cend(), cmp);
    assert(it != materials.cend());
    auto const material = it->material;
    auto const color    = load_color_or_abort(file, "color");
    return TileInfo{tiletype, mesh_name, vshader_name, color, material};
  };
  auto const  tile_table = get_table_array(table, "tile");
  auto const& ttable     = tile_table->as_table_array()->get();

  // Ensure we load data for everry tile
  assert(TileSharedInfoTable::SIZE == ttable.size());

  std::array<TileInfo, TileSharedInfoTable::SIZE> tinfos;
  FOR(i, ttable.size())
  {
    auto const& it   = ttable[i];
    auto        tile = load_tile(it);
    tinfos[i]        = MOVE(tile);
  }
  return TileSharedInfoTable{MOVE(tinfos)};
}

using LoadResult = Result<std::pair<std::string, opengl::ShaderProgram>, std::string>;
LoadResult
load_shader(stlw::Logger& logger, ParsedVertexAttributes& pvas, CppTable const& table)
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
load_shaders(stlw::Logger& logger, ParsedVertexAttributes&& pvas, CppTable const& table)
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

    auto const uint_index     = static_cast<GLuint>(index);
    auto const attribute_type = attribute_type_from_string(fieldname);
    auto       api = opengl::AttributePointerInfo{uint_index, datatype, attribute_type, num};

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

} // namespace

namespace boomhs
{

// This macro exists to reduce code duplication implementing the two different implementation of
// operator[].
#define SEARCH_FOR(type, begin, end)                                                               \
  auto const cmp = [&type](auto const& tinfo) { return tinfo.type == type; };                      \
  auto const it  = std::find_if(begin, end, cmp);                                                  \
  assert(it != end);                                                                               \
  return *it;

////////////////////////////////////////////////////////////////////////////////////////////////////
// TileSharedInfoTable
TileInfo const& TileSharedInfoTable::operator[](TileType const type) const
{
  SEARCH_FOR(type, data_.cbegin(), data_.cend());
}

TileInfo& TileSharedInfoTable::operator[](TileType const type)
{
  SEARCH_FOR(type, data_.begin(), data_.end());
}
#undef SEARCH_FOR

///////////////////////////////////////////////////////////////////////////////////////////////////
// LevelLoader
Result<LevelAssets, std::string>
LevelLoader::load_level(stlw::Logger& logger, EntityRegistry& registry, std::string const& filename)
{
  CppTable engine_table = cpptoml::parse_file("engine.toml");
  assert(engine_table);

  ParsedVertexAttributes pvas = load_vas(engine_table);
  auto                   sps  = TRY_MOVEOUT(load_shaders(logger, MOVE(pvas), engine_table));

  CppTable file_datatable = cpptoml::parse_file("levels/" + filename);
  assert(file_datatable);

  auto const mesh_table = get_table_array_or_abort(file_datatable, "meshes");
  ObjStore   objstore =
      TRY_MOVEOUT(load_objfiles(logger, mesh_table).mapErrorMoveOut(loadstatus_to_string));

  LOG_TRACE("loading textures ...");
  auto texture_table = TRY_MOVEOUT(load_textures(logger, file_datatable));

  LOG_TRACE("loading orbital data");
  auto const orbital_bodies = load_orbital_bodies(logger, file_datatable, registry);

  LOG_TRACE("loading material data");
  auto const materials = load_materials(logger, file_datatable, registry);

  LOG_TRACE("loading attenuation data");
  auto const attenuations = load_attenuations(logger, file_datatable, registry);

  LOG_TRACE("loading pointlight data");
  auto const pointlights = load_pointlights(logger, file_datatable, attenuations);

  LOG_TRACE("loading entities ...");
  load_entities(logger, file_datatable, orbital_bodies, texture_table, materials, pointlights,
                registry);

  LOG_TRACE("loading tile materials ...");
  auto tile_table = load_tileinfos(logger, file_datatable, materials, registry);

  LOG_TRACE("loading lights ...");
  auto const ambient = Color{get_vec3_or_abort(file_datatable, "ambient")};
  auto const directional_light_diffuse =
      Color{get_vec3_or_abort(file_datatable, "directional_light_diffuse")};
  auto const directional_light_specular =
      Color{get_vec3_or_abort(file_datatable, "directional_light_specular")};
  auto const directional_light_direction =
      get_vec3_or_abort(file_datatable, "directional_light_direction");

  Light            light{directional_light_diffuse, directional_light_specular};
  DirectionalLight dlight{MOVE(light), directional_light_direction};
  GlobalLight      glight{ambient, MOVE(dlight)};

  auto const fog = load_fog(file_datatable);
  LOG_TRACE("yielding assets");
  return Ok(LevelAssets{MOVE(glight), fog,

                        MOVE(tile_table),

                        MOVE(objstore), MOVE(texture_table), MOVE(sps)});
}

} // namespace boomhs
