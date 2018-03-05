#include <cstdlib>
#include <experimental/filesystem>
#include <iostream>
#include <stlw/optional.hpp>
#include <stlw/os.hpp>

namespace fs = std::experimental::filesystem;
using OptionalString = std::optional<std::string>;

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
copy_to_outdir(std::string const& prefix, fs::path const &shader_path, fs::path const &outdir)
{
  auto const shader_read_result = stlw::read_file(shader_path.string().c_str());
  if (!shader_read_result) {
    return false;
  }
  auto const shader_contents = shader_read_result.expect("shader contents");
  auto const output_shaderfile_name = outdir.string() + shader_path.filename().string();

  // 1) try and create the directory, if it exists already who cares.
  fs::create_directory(outdir);

  // 2) remove the file if it exists already (we don't care about success here, unless
  // failure to delete becomes a problem someday ...)
  delete_if_exists(output_shaderfile_name);

  // 3) now write the modified copy out
  stlw::write_file(output_shaderfile_name, prefix, shader_contents);
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

constexpr char const* prefix_contents = R"(#version 300 es
precision mediump float;
)";

int
main(int argc, char *argv[])
{
  auto &log = std::cerr;
  auto const paths = make_paths(log);
  auto const &CWD = paths.first;
  auto const &path_to_shaders = paths.second;

  auto const on_error = [](auto const msg) {
    log << "Error running shader loader, problem: '" << msg << "'";
    return EXIT_FAILURE;
  };
  if (!fs::exists(path_to_shaders)) {
    auto const msg = "no shader at path '" + path_to_shaders.string() + "' exists.";
    return on_error(msg);
  } else if (!fs::is_directory(path_to_shaders)) {
    return on_error("found 'shader' to not be a directory at this path.");
  }

  auto const iter_directories = [&path_to_shaders](auto const& fn) {
    bool result = true;
    for (fs::directory_iterator it{path_to_shaders}; it != fs::directory_iterator{}; ++it) {
      result &= fn(it);
    }
    return result;
  };

  // Make a pass over the directory, building up our "glsl library" code
  std::string library_code;
  auto const build_library = [&library_code](auto const& directory_iter) {
    auto const path = directory_iter->path();
    bool const is_libraryfile = path.filename().string().find(".glsl") != std::string::npos;
    if (is_libraryfile) {
      auto const read_result = stlw::read_file(path.string().c_str());
      if (!read_result) {
        return false;
      }
      library_code += read_result.expect("file contents");
    }
    return true;
  };

  auto const copy_files = [&CWD, &library_code](auto const& it) {
    auto const outdir = CWD.string() + "/build-system/bin/shaders/";
    auto const path = it->path();
    auto const path_string = path.filename().string();

    // Don't copy library files themselves to the output directory.
    bool const dont_copy = path_string.find(".glsl") != std::string::npos;
    if (dont_copy) {
      return true;
    }

    // For now, vertex shaders don't share code
    bool const is_vertexshader = path_string.find(".vert") != std::string::npos;
    auto const contents = is_vertexshader ? prefix_contents : prefix_contents + library_code;
    if (!copy_to_outdir(contents, path, outdir)) {
      return false;
    }
    return true;
  };

  bool result = iter_directories(build_library);
  result |= iter_directories(copy_files);
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
