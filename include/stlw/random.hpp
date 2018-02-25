#pragma once
#include <random>

#include <stlw/type_macros.hpp>
#include <stlw/math.hpp>

namespace stlw
{

// TODO: This generates more than floats...
class float_generator {
  uint64_t const seed_;
  std::mt19937 generator_;

  static auto make_seed()
  {
    uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq seeder{uint32_t(seed),uint32_t(seed >> 32)};
    ++seed;
    int out;
    seeder.generate(&out, &out+1);
    return static_cast<uint32_t>(out);
  }
public:
  MOVE_CONSTRUCTIBLE_ONLY(float_generator);
  explicit float_generator()
    : seed_(make_seed())
    , generator_(this->seed_)
  {
  }

  float gen_negative1to1()
  {
    auto constexpr FROM = std::make_pair(-255, 255);
    auto constexpr TO = std::make_pair(-1.0f, 1.0f);
    std::uniform_int_distribution<int> distribution(FROM.first, FROM.second);
    int const value = distribution(this->generator_);
    return stlw::math::normalize(value, FROM, TO);
  }

  float gen_0to1()
  {
    auto constexpr FROM = std::make_pair(0, 255);
    auto constexpr TO = std::make_pair(0.0f, 1.0f);
    std::uniform_int_distribution<int> distribution(FROM.first, FROM.second);
    int const value = distribution(this->generator_);
    return stlw::math::normalize(value, FROM, TO);
  }

  auto gen_bool()
  {
    std::uniform_int_distribution<int> distribution(0, 1);
    auto const int_value = distribution(this->generator_);
    return int_value > 0 ? true : false;
  }

  using range2d = std::pair<float, float>;
  auto gen_position(range2d const& from)
  {
    std::uniform_real_distribution<float> distribution{from.first, from.second};
    return distribution(this->generator_);
  }

  using range3d = std::array<float, 3>;
  auto gen_3dposition(range3d const& lower, range3d const& upper)
  {
    auto const x = gen_position({lower[0], upper[0]});
    auto const y = gen_position({lower[1], upper[1]});
    auto const z = gen_position({lower[2], upper[2]});
    return std::array<float, 3>{x, y, z};
  }

  auto gen_3dposition_above_ground()
  {
    auto const LOWER = std::array<float, 3>{0, 1, 0};
    auto const UPPER = std::array<float, 3>{10, 5, 10};
    return gen_3dposition(LOWER, UPPER);
  }

  auto gen_2dposition()
  {
    auto const x = gen_position({-1.0f, 1.0f});
    auto const y = gen_position({-1.0f, 1.0f});
    return std::make_pair(x, y);
  }

  auto gen_float_range(float const low, float const high)
  {
    assert(low <= high);
    std::uniform_real_distribution<float> distribution{low, high};
    return distribution(this->generator_);
  }

  auto gen_int_range(int const low, int const high)
  {
    assert(low <= high);
    std::uniform_int_distribution<int> distribution{low, high};
    return distribution(this->generator_);
  }

  auto gen_uint64_range(uint64_t const low, uint64_t const high)
  {
    assert(low <= high);
    std::uniform_int_distribution<uint64_t> distribution{low, high};
    return distribution(this->generator_);
  }
};

} // ns stlw
