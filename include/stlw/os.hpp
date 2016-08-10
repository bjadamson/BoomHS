#pragma once
#include <fstream>
#include <stlw/result.hpp>
#include <string>

namespace stlw
{

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

} // ns stlw
