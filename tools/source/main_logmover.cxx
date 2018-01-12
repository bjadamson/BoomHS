#include <tools/common.hpp>
#include <stlw/format.hpp>
#include <stlw/result.hpp>

#include <boost/optional.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

using tools::RelaventPaths;

namespace
{

std::string static const LOG_MATCH = ".log_";
std::string static const LASTRUN_DIRNAME_PREFIX = "LastRun#";

std::pair<bool, int>
count_matches(RelaventPaths const& paths)
{
  int count{0};
  bool found{false};
  auto const count_fn = [&](auto const& it) {
    std::string const filename = it->path().filename();
    if (boost::starts_with(filename, LASTRUN_DIRNAME_PREFIX)) {
      ++count;
    }

    if (boost::contains(filename, LOG_MATCH)) {
      found = true;
    }
  };

  tools::foreach_path(paths.bindir, count_fn);
  return std::make_pair(found, count);
}

stlw::result<int, std::string>
move_logfiles(RelaventPaths const& paths)
{
  // 1) Find the longest directory matching "LastRun+X" where X is an integer in the domain 0..inf
  auto const [found, count] = count_matches(paths);
  if (!found) {
    // No need to do anything if we can't find any log files.
    return EXIT_SUCCESS;
  }

  auto const path = fmt::sprintf("%s/%s%s/",
      paths.bindir.string(),
      LASTRUN_DIRNAME_PREFIX,
      std::to_string(count));
  auto const new_dir = fs::path{path};
  fs::create_directory(new_dir);

  auto const move_file_fn = [&new_dir](auto const& it) {
    // If it points to a directory or a file that isn't a log file, we skip.
    if (fs::is_directory(*it)) {
      return;
    }
    std::string const filename = it->path().filename();
    if (!boost::contains(filename, LOG_MATCH)) {
      return;
    }

    // File isn't a directory, so move it.
    tools::move_to_outdir("", it->path(), new_dir);
  };
  tools::foreach_path(paths.bindir, move_file_fn);
  return EXIT_SUCCESS;
}

} // ns anon

int
main(int argc, char *argv[])
{
  auto &log = std::cerr;
  auto const paths = tools::make_paths(log);

  auto const on_error = [&log](auto const msg) {
    log << "Error running log mover, problem: '" << msg << "'";
    return EXIT_FAILURE;
  };
  if (!fs::exists(paths.bindir)) {
    return on_error("No bin directory.");
  }

  // TODO: try and reuse status
  DO_TRY_OR_ELSE_RETURN(int status, move_logfiles(paths), on_error);
  return status;
}
