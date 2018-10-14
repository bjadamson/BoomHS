#include <common/algorithm.hpp>
#include <common/compiler.hpp>
#include <common/log.hpp>
#include <common/type_macros.hpp>

#include <iostream>
#include <memory>

using namespace common;
using namespace common::impl;

namespace
{

auto
threadunsafe_sink(char const* file_path)
{
  // use single-threaded spdlog sink
  using SinkType = spdlog::sinks::simple_file_sink_st;
  return std::make_unique<SinkType>(file_path);
}

auto
threadsafe_sink(char const* file_path)
{
  // use multi-threaded spdlog sink
  using SinkType = spdlog::sinks::simple_file_sink_mt;
  return std::make_unique<SinkType>(file_path);

  // using SinkType = spdlog::sinks::daily_file_sink_st;
  // return std::make_unique<SinkType>(file_path, 23, 50);
}

auto
threadsafe_stderr_sink()
{
  using SinkType = spdlog::sinks::stderr_sink_st;
  return std::make_unique<SinkType>();
}

LogFlusher
make_logflusher(char const* file_path, spdlog::level::level_enum const level)
{
  try {
    auto file_sink   = threadsafe_sink(file_path);
    auto stderr_sink = threadsafe_stderr_sink();
    stderr_sink->set_level(spdlog::level::err);

    auto const sinks  = common::make_array<spdlog::sink_ptr>(MOVE(file_sink), MOVE(stderr_sink));
    auto       logger = std::make_unique<spdlog::logger>(file_path, sinks.cbegin(), sinks.cend());
    logger->set_level(level);

    auto adapter = impl::LogAdapter{MOVE(logger)};
    return LogFlusher{MOVE(adapter)};
  }
  catch (spdlog::spdlog_ex const& ex) {
    std::cerr << ex.what() << "\n";
    std::abort();
  }
}

} // namespace

namespace common
{

Logger
LogFactory::make_default(char const* name)
{
  // 1. Construct an instance of the default log group.
  // 2. Construct an instance of a logger that writes all log levels to a shared file.
  static char const prefix[] = "build-system/bin/logs/";
  auto const        path     = prefix + std::string{name};
  auto              flusher  = make_logflusher(path.c_str(), spdlog::level::trace);
  return Logger{MOVE(flusher)};
}

Logger
LogFactory::make_stderr()
{
  auto const level = spdlog::level::err;
  try {
    auto stderr_sink = threadsafe_stderr_sink();
    stderr_sink->set_level(level);

    auto const sinks  = common::make_array<spdlog::sink_ptr>(MOVE(stderr_sink));
    auto       logger = std::make_unique<spdlog::logger>("", sinks.cbegin(), sinks.cend());
    logger->set_level(level);

    auto adapter = impl::LogAdapter{MOVE(logger)};
    return Logger{LogFlusher{MOVE(adapter)}};
  }
  catch (spdlog::spdlog_ex const& ex) {
    std::cerr << ex.what() << "\n";
    std::abort();
  }
}

} // namespace common
