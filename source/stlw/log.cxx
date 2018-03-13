#include <stlw/log.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/compiler_macros.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <iostream>
#include <memory>

using namespace stlw;
using namespace stlw::impl;

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

  //using SinkType = spdlog::sinks::daily_file_sink_st;
  //return std::make_unique<SinkType>(file_path, 23, 50);
}

LogFlusher
make_aggregate_logger(char const *file_path, spdlog::level::level_enum const level)
{
  try {
    auto const sinks = stlw::make_array<spdlog::sink_ptr>(
        threadsafe_sink(file_path)
        );
    auto logger = std::make_unique<spdlog::logger>(file_path, sinks.cbegin(), sinks.cend());
    logger->set_level(level);
    auto adapter = impl::LogAdapter{MOVE(logger)};
    return LogFlusher{MOVE(adapter)};
  }
  catch (spdlog::spdlog_ex const& ex) {
    std::cerr << ex.what() << "\n";
    std::abort();
  }
}

} // ns anon

namespace stlw
{

impl::LogWriter
LogFactory::make_default(char const *name)
{
  // 1. Construct an instance of the default log group.
  // 2. Construct an instance of a logger that writes all log levels to a shared file.
  static char const prefix[] = "build-system/bin/";
  auto const path = prefix + std::string{name} + ".log";
  auto ad = make_aggregate_logger(path.c_str(), spdlog::level::trace);
  return impl::LogWriter{MOVE(ad)};
}

} // ns stlw
