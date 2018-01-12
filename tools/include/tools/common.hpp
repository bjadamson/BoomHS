#pragma once
#include <experimental/filesystem>

#include <string>

namespace fs = std::experimental::filesystem;

namespace tools
{

struct RelaventPaths
{
  fs::path const CWD;
  fs::path const bindir;
  fs::path const shaders;
};

template<typename FN>
void
foreach_path(fs::path const& path, FN const& fn)
{
  for (fs::directory_iterator it{path}; it != fs::directory_iterator{}; ++it) {
    fn(it);
  }
}

RelaventPaths
make_paths(std::ostream &);

bool
move_to_outdir(std::string const &prefix, fs::path const &path, fs::path const &outdir);

} // ns tools
