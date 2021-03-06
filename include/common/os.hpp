#pragma once
#include <common/result.hpp>
#include <common/tuple.hpp>
#include <common/type_macros.hpp>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace common
{

// TODO: boost::path
inline Result<std::string, std::string>
read_file(char const* path)
{
  // Read the Vertex Shader code from the file
  std::stringstream sstream;
  {
    std::ifstream istream(path, std::ios::in);

    if (!istream.is_open()) {
      return Err("Error opening file at path '" + std::string{path} + "'");
    }

    std::string buffer;
    bool        first = true;
    while (std::getline(istream, buffer)) {
      if (!first) {
        sstream << "\n";
      }
      else {
        first = false;
      }
      sstream << buffer;
    }
    // explicit, dtor should do this.
    istream.close();
  }
  return Ok(sstream.str());
}

template <typename T>
auto
read_file(T const s)
{
  return read_file(s.c_str());
}

// TODO: boost::path
template <typename... Text>
void
write_file(std::experimental::filesystem::path const& path, Text const&... text)
{
  std::filebuf fb;
  fb.open(path.string().c_str(), std::ios::out);
  ON_SCOPE_EXIT([&fb]() { fb.close(); });

  std::ostream os(&fb);
  auto const   write_line = [&os](auto const& text) { os << text; };

  auto const tuple_view = std::make_tuple(text...);
  common::tuple_for_each(tuple_view, write_line);
}

} // namespace common
