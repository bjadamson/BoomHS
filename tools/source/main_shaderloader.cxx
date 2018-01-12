#include <tools/common.hpp>
#include <stlw/result.hpp>
#include <iostream>

using tools::RelaventPaths;

namespace
{

stlw::result<int, std::string>
copy_shaders(RelaventPaths const& paths)
{
  if (!fs::exists(paths.shaders)) {
    auto const msg = "no shader at path '" + paths.shaders.string() + "' exists.";
    return ERROR(msg);
  } else if (!fs::is_directory(paths.shaders)) {
    return ERROR(std::string{"found 'shader' to not be a directory at this path."});
  }

  constexpr char const* prefix = R"(#version 300 es
precision mediump float;)";

  for (fs::directory_iterator it{paths.shaders}; it != fs::directory_iterator{}; ++it) {
    auto const outdir = paths.bindir.string() + "/shaders/";
    if (!tools::move_to_outdir(prefix, it->path(), outdir)) {
      return ERROR(std::string{"Error copying to outdirectory (investigation needed)."});
    }
  }
  return EXIT_SUCCESS;
}

} // ns anonymous

int
main(int argc, char *argv[])
{
  auto &log = std::cerr;
  auto const paths = tools::make_paths(log);

  auto const on_error = [&log](auto const msg) {
    log << "Error running shader loader/log mover, problem: '" << msg << "'";
    return EXIT_FAILURE;
  };
  if (!fs::exists(paths.bindir)) {
    return on_error("No bin directory.");
  }

  // TODO: try and reuse status
  DO_TRY_OR_ELSE_RETURN(int status, copy_shaders(paths), on_error);
  return status;
}
