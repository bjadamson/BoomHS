#pragma once
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <stlw/result.hpp>
#include <stlw/tuple.hpp>
#include <stlw/type_macros.hpp>
#include <string>

namespace stlw
{

// TODO: boost::path
stlw::result<std::string, std::string>
read_file(char const *path)
{
  // Read the Vertex Shader code from the file
  std::stringstream sstream;
  {
    std::ifstream istream(path, std::ios::in);

    if (!istream.is_open()) {
      return stlw::make_error("Error opening file at path '" + std::string{path} + "'");
    }

    std::string next_line = "";
    while (std::getline(istream, next_line)) {
      sstream << "\n" << next_line;
    }
    // explicit, dtor should do this.
    istream.close();
  }
  return sstream.str();
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
write_file(std::experimental::filesystem::path const &path, Text const &... text)
{
  std::filebuf fb;
  fb.open(path.string().c_str(), std::ios::out);
  ON_SCOPE_EXIT([&fb]() { fb.close(); });

  std::ostream os(&fb);
  auto const write_line = [&os](auto const &text) { os << text; };

  auto const tuple_view = std::make_tuple(text...);
  stlw::for_each(tuple_view, write_line);
}

} // ns stlw
