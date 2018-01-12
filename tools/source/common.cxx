#include <tools/common.hpp>
#include <stlw/os.hpp>
#include <cstdlib>
#include <iostream>

namespace
{

void
delete_if_exists(fs::path const &path)
{
  if (fs::exists(path)) {
    fs::remove(path);
  }
}

} // ns anonymous

namespace tools
{

RelaventPaths
make_paths(std::ostream &log)
{
  auto const determine_root = []() {
    auto const CWD = fs::current_path();
    if (auto const it = CWD.string().find("build-system"); it != std::string::npos) {
      return CWD.parent_path();
    }
    return CWD;
  };

  auto const CWD = determine_root();
  fs::path const shaders{std::string{CWD} + "/shaders/"};
  std::string const bindir = CWD.string() + "/build-system/bin";

  log << "CWD: '" << CWD << "'" << std::endl;
  log << "bindir: '" << bindir << "'" << std::endl;
  log << "shaders: '" << shaders.string() << "'" << std::endl;
  return RelaventPaths{CWD, bindir, shaders};
}

bool
move_to_outdir(std::string const &prefix, fs::path const &path, fs::path const &outdir)
{
  auto const read_result = stlw::read_file(path.string().c_str());
  if (!read_result) {
    return false;
  }
  auto const file_contents = *read_result;
  auto const output_filename = outdir.string() + path.filename().string();

  // 1) try and create the directory, if it exists already who cares.
  fs::create_directory(outdir);

  // 2) remove the file if it exists already (we don't care about success here, unless
  // failure to delete becomes a problem someday ...)
  delete_if_exists(output_filename);

  // 3) now write the modified copy out
  stlw::write_file(output_filename, prefix, file_contents);
  return true;
}

} // ns tools
