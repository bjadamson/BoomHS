#pragma once
#include <fstream>
#include <string>
#include <boost/expected/expected.hpp>

namespace stlw
{

boost::expected<std::string, std::string>
read_file(char const* path)
{
  // Read the Vertex Shader code from the file
  std::ifstream istream(path, std::ios::in);
  std::stringstream sstream;

  if (! istream.is_open()) {
    return boost::make_unexpected("Error opening file at path '" + std::string{path} + "'");
  }

  std::string next_line = "";
  while(std::getline(istream, next_line)) {
    sstream << "\n" << next_line;
  }
  // explicit, dtor should do this.
  istream.close();
  return sstream.str();
}

} // ns stlw
