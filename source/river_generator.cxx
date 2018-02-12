#include <boomhs/river_generator.hpp>
#include <boomhs/tiledata.hpp>
#include <boomhs/tiledata_algorithms.hpp>

#include <stlw/optional.hpp>
#include <stlw/random.hpp>
#include <stlw/algorithm.hpp>

using namespace boomhs;

namespace
{

void
spawn_newround_wiggles(RiverInfo &river, stlw::float_generator &rng, glm::vec2 const pos)
{
  float const speed    = 100.0f;
  float const OFFSET   = 0.10f;
  float const x_offset  = rng.gen_float_range(-OFFSET, OFFSET);
  float const z_offset  = rng.gen_float_range(-OFFSET, OFFSET);
  auto const offset = glm::vec2{x_offset, z_offset};
  RiverWiggle wiggle{speed, offset, pos, river.flow_direction};
  river.wiggles.emplace_back(MOVE(wiggle));
}

RiverInfo
create_river(TilePosition const& tpos, Edges const& edges, glm::vec2 const& flow_dir,
    float const rotation, float const length, stlw::float_generator &rng)
{
  RiverInfo river{tpos, edges.left, edges.top, edges.right, edges.bottom, flow_dir, rotation};
  FOR(i, length) {
    auto const offset = (flow_dir * static_cast<float>(i));
    glm::vec2 const wiggle_pos = static_cast<glm::vec2>(river.origin) + offset;
    spawn_newround_wiggles(river, rng, wiggle_pos);
  }
  return river;
}

stlw::optional<RiverInfo>
generate_river(TileData &tdata, stlw::float_generator &rng)
{
  auto const [tdwidth, tdheight] = tdata.dimensions();
  auto const tpos_edge = random_tileposition_onedgeofmap(tdata, rng);
  auto const& tpos = tpos_edge.first;
  MapEdge const& edge = tpos_edge.second;

  auto const RIVER_DISTANCE = 1;
  if (edge.is_xedge()) {
    FOR(i, tdwidth) {
      tdata.data(i, tpos.y).type = TileType::RIVER;
    }

    auto constexpr FLOW_DIR = glm::vec2{1.0, 0.0};
    auto const edges = calculate_edges(tpos, tdwidth, tdheight, tdwidth, RIVER_DISTANCE);
    float const rotation = 90.0f;

    auto const left = glm::vec2{static_cast<float>(edges.left), 0.0f};
    auto const right = glm::vec2{static_cast<float>(edges.right), 0.0f};
    auto const length = glm::distance(left, right);
    return create_river(tpos, edges, FLOW_DIR, rotation, length, rng);
  }
  assert(edge.is_yedge());
  FOR(i, tdheight) {
    tdata.data(tpos.x, i).type = TileType::RIVER;
  }

  auto constexpr FLOW_DIR = glm::vec2{0.0, 1.0};
  auto const edges = calculate_edges(tpos, tdwidth, tdheight, RIVER_DISTANCE, tdheight);
  float const rotation = 0.0f;

  auto const top = glm::vec2{0.0f, static_cast<float>(edges.top)};
  auto const bottom = glm::vec2{0.0f, static_cast<float>(edges.bottom)};
  auto const length = glm::distance(top, bottom);
  return create_river(tpos, edges, FLOW_DIR, rotation, length, rng);
}

} // ns anon

namespace boomhs::river_generator
{

void
place_rivers(TileData &tdata, stlw::float_generator &rng, std::vector<RiverInfo> &rivers)
{
  auto const place = [&]() {
    stlw::optional<RiverInfo> river_o = stlw::none;
    while(!river_o) {
      river_o = generate_river(tdata, rng);
    }
    assert(river_o);
    auto river = MOVE(*river_o);
    rivers.emplace_back(MOVE(river));
  };

  FOR(i, 2) {
    place();
  }
}

} // ns boomhs::river_generator
