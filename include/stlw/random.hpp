#pragma once
#include <chrono>
#include <random>

#include <stlw/math.hpp>

namespace stlw
{

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
  explicit float_generator()
    : seed_(make_seed())
    , generator_(this->seed_)
  {
  }

  float generate_0to1()
  {
    auto constexpr FROM = std::make_pair(0, 255);
    auto constexpr TO = std::make_pair(0.0f, 1.0f);
    std::uniform_int_distribution<int> distribution(FROM.first, FROM.second);
    int const value = distribution(this->generator_);
    return stlw::math::normalize(value, FROM, TO);
  }

  auto generate_bool()
  {
    std::uniform_int_distribution<int> distribution(0, 1);
    auto const int_value = distribution(this->generator_);
    return int_value > 0 ? true : false;
  }

  auto generate_position()
  {
    auto constexpr FROM = std::make_pair(-1000, 1000);
    auto constexpr TO = std::make_pair(-1.0f, 1.0f);
    std::uniform_int_distribution<int> distribution(FROM.first, FROM.second);
    auto const int_value = distribution(this->generator_);
    return stlw::math::normalize(int_value, FROM, TO);
  }
};

} // ns stlw
