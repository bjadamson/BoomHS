#pragma once
#include <stlw/types.hpp>
#include <vector>

namespace stlw
{

// TODO: make this a template template fn?
template <typename T1>
void
print(std::ostream &out, std::vector<T1> const &c, char const *delim = "[,]",
      bool const newline = true)
{
  out << delim[0];
  if (!c.empty()) {
    for (auto t = c.begin(); t != c.end() - 1; ++t) {
      out << *t << delim[1] << " ";
    }
    // print the last element separately to avoid the extra characters following it.
    out << *--c.end();
  }
  out << delim[2];
  if (newline) {
    out << "\n";
  }
}

} // ns stlw
