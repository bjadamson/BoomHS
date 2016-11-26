#include <cstdlib>
#include <experimental/filesystem>
#include <iostream>
#include <stlw/os.hpp>

namespace fs = std::experimental::filesystem;

namespace
{

void
delete_if_exists(fs::path const &path)
{
  if (fs::exists(path)) {
    fs::remove(path);
  }
}

bool
copy_to_outdir(std::string const &contents, fs::path const &shader_path, fs::path const &outdir)
{
  auto const shader_read_result = stlw::read_file(shader_path.string().c_str());
  if (!shader_read_result) {
    return false;
  }
  auto const shader_contents = *shader_read_result;
  auto const output_shaderfile_name = outdir.string() + shader_path.filename().string();

  // 1) try and create the directory, if it exists already who cares.
  fs::create_directory(outdir);

  // 2) remove the file if it exists already (we don't care about success here, unless
  // failure to delete becomes a problem someday ...)
  delete_if_exists(output_shaderfile_name);

  // 3) now write the modified copy out
  stlw::write_file(output_shaderfile_name, contents, shader_contents);
  return true;
}

} // ns anonymous

template <typename L>
auto
make_paths(L &log)
{
  auto const determine_root = []() {
    auto const CWD = fs::current_path();
    if (auto const it = CWD.string().find("build-system"); it != std::string::npos) {
      return CWD.parent_path();
    }
    return CWD;
  };

  auto const CWD = determine_root();
  fs::path const path_to_shaders{std::string{CWD} + "/shaders/"};
  log << "CWD: '" << CWD << "'" << std::endl;
  log << "path_to_shaders: '" << path_to_shaders.string() << "'" << std::endl;

  return std::make_pair(CWD, path_to_shaders);
}

int
main(int argc, char *argv[])
{
  auto &log = std::cerr;
  auto const paths = make_paths(log);
  auto const &CWD = paths.first;
  auto const &path_to_shaders = paths.second;

  auto const on_error = [&log](auto const msg) {
    log << "Error running shader loader, problem: '" << msg << "'";
    return EXIT_FAILURE;
  };
  if (!fs::exists(path_to_shaders)) {
    auto const msg = "no shader at path '" + path_to_shaders.string() + "' exists.";
    return on_error(msg);
  } else if (!fs::is_directory(path_to_shaders)) {
    return on_error("found 'shader' to not be a directory at this path.");
  }

  // preamble
  auto const preamble_read_result = stlw::read_file("./include/engine/gfx/opengl/glsl.hpp");
  if (!preamble_read_result) {
    return false;
  }
  auto const preamble = *preamble_read_result;
  auto const glsl_version = R"(#version 300 es)";
  auto const combined = glsl_version + preamble;

  for (fs::directory_iterator it{path_to_shaders}; it != fs::directory_iterator{}; ++it) {
    auto const outdir = CWD.string() + "/build-system/bin/shaders/";
    if (!copy_to_outdir(combined, it->path(), outdir)) {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
