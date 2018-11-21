#include <common/algorithm.hpp>
#include <common/optional.hpp>
#include <common/os.hpp>
#include <opengl/global.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <boost/algorithm/string/predicate.hpp>

namespace fs = std::filesystem;
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
copy_to_outdir(std::string const& contents, fs::path const &shader_path, fs::path const &outdir)
{
  auto const shader_read_result = common::read_file(shader_path.string().c_str());
  if (!shader_read_result) {
    return false;
  }
  auto const shader_contents = shader_read_result.expect("shader contents");
  auto const output_shaderfile_name = outdir.string() + shader_path.filename().string();

  // remove the file if it exists already (we don't care about success here, unless
  // failure to delete becomes a problem someday ...)
  delete_if_exists(output_shaderfile_name);

  // write the modified copy out to the file
  common::write_file(output_shaderfile_name, contents, shader_contents);
  return true;
}

struct Paths
{
  fs::path const read_shaders_path, write_shaders_path;
  fs::path const log_path;
};

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

  auto const root = determine_root();
  auto const rootdir = root.string();
  auto const bindir = rootdir + "/build-system/bin";

  fs::path const read_shaders_path{rootdir + "/shaders/"};
  fs::path const write_shaders_path{bindir + "/shaders/"};

  fs::path const log_path{bindir + "/logs/"};
  return Paths{read_shaders_path, write_shaders_path, log_path};
}

} // ns anonymous

template<typename FN, typename ...Args>
auto
iter_directories(fs::path const& path_to_shaders, FN const& fn, Args &&... args)
{
  bool result = true;
  for (fs::directory_iterator it{path_to_shaders}; it != fs::directory_iterator{}; ++it) {
    result &= fn(it, FORWARD(args));
  }
  return result;
}

auto
get_library_code(fs::path const& path_to_shaders, char const* file_extension)
{
  std::string library_code;
  auto const build_library = [&](auto const& directory_iter) {
    auto const path = directory_iter->path();
    bool const is_libraryfile = path.filename().string().find(file_extension) != std::string::npos;
    if (is_libraryfile) {
      auto const read_result = common::read_file(path.string().c_str());
      if (!read_result) {
        return false;
      }
      library_code += read_result.expect("file contents");
    }
    return true;
  };

  bool const result = iter_directories(path_to_shaders, build_library);
  return PAIR(result, library_code);
}

OptionalString
copy_shaders_to_outdir(fs::path const& path_to_shaders, std::string const& outdir)
{
  if (!fs::exists(path_to_shaders)) {
    return "no shader at path '" + path_to_shaders.string() + "' exists.";
  } else if (!fs::is_directory(path_to_shaders)) {
    return "found 'shader' to not be a directory at this path.";
  }

  auto const copy_files = [&outdir](auto const& it, auto const& library_code,
      char const* extension_to_skip)
  {
    auto const path = it->path();
    auto const path_string = path.filename().string();

    // Don't copy library files themselves to the output directory.
    bool const dont_copy = path_string.find(".glsl") != std::string::npos;
    if (dont_copy) {
      return true;
    }

    // Don't copy this type also
    if (std::string::npos != path_string.find(extension_to_skip)) {
      return true;
    }

    if (!copy_to_outdir(library_code, path, outdir)) {
      return false;
    }
    return true;
  };

  // Iterate over the directory
  //
  // first, collect all the shared code into two strings (one per vert/frag shader) each prefixed
  // with the expected contents.
  char const* PREFIX_STRING = opengl::glsl::prefix_string();
  auto const both_libcode_pair = get_library_code(path_to_shaders, ".glsl_both");
  bool result = both_libcode_pair.first;

  // second collect all the vertex shader shared code.
  auto const vert_libcode_pair = get_library_code(path_to_shaders, ".glsl_vert");
  result |= vert_libcode_pair.first;

  // third collect all the fragment shader shared code.
  auto const frag_libcode_pair = get_library_code(path_to_shaders, ".glsl_frag");
  result |= frag_libcode_pair.first;

  auto const prefix_both_libcode = PREFIX_STRING       + both_libcode_pair.second;
  auto const frag_libcode        = prefix_both_libcode + frag_libcode_pair.second;
  auto const vert_libcode        = prefix_both_libcode + vert_libcode_pair.second;

  // Copy the shaders (prepended with their shared code) to the output directories.
  result |= iter_directories(path_to_shaders, copy_files, frag_libcode, ".vert");
  result |= iter_directories(path_to_shaders, copy_files, vert_libcode, ".frag");
  return result ? std::nullopt : std::make_optional("Error copying files.");
}

int
main(int argc, char *argv[])
{
  auto &log = std::cerr;
  auto const paths = make_paths(log);
  auto const &read_shaders_path = paths.read_shaders_path;
  auto const &write_shaders_path = paths.write_shaders_path;
  auto const &log_path = paths.log_path;

  auto const on_error = [](auto const msg) {
    log << "Error running post-build program, problem: '" << msg << "'";
    return EXIT_FAILURE;
  };
  {
    auto const write_dir = write_shaders_path.string();
    fs::create_directory(write_dir);

    auto const read_dir = read_shaders_path.string();
    auto const result = copy_shaders_to_outdir(read_dir, write_dir);
    if (result) {
      return on_error(*result);
    }
    log << "Shaders successfully copied.\n";
  }

  fs::create_directory(log_path);
  return EXIT_SUCCESS;
}
