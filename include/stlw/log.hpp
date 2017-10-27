#pragma once
#include <memory>
#include <stlw/algorithm.hpp>
#include <stlw/compiler_macros.hpp>
#include <stlw/impl/log_impl.hpp>
#include <stlw/type_macros.hpp>

#undef LOG_TRACE
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR

#ifdef NDEBUG
#define LOG_TRACE(...) logger.log_nothing()
#define LOG_DEBUG(...) logger.log_nothing()
#define LOG_INFO(...) logger.log_nothing()
#define LOG_WARN(...) logger.log_nothing()
#define LOG_ERROR(...) logger.log_nothing()
#else
#define LOG_TRACE(...) logger.trace(__VA_ARGS__)
#define LOG_DEBUG(...) logger.debug(__VA_ARGS__)
#define LOG_INFO(...) logger.info(__VA_ARGS__)
#define LOG_WARN(...) logger.warn(__VA_ARGS__)
#define LOG_ERROR(...) logger.error(__VA_ARGS__)
#endif

namespace stlw
{

class log_factory
{
  template <typename... Params>
  inline static auto
  make_spdlog_logger(char const *name, impl::log_level const level, Params &&... p)
  {
    auto sink_ptr = std::make_unique<spdlog::sinks::daily_file_sink_st>(std::forward<Params>(p)...);
    auto log_impl_pointer = std::make_unique<spdlog::logger>(name, std::move(sink_ptr));
    {
      // map abstract log levels to spdlog levels
      auto const log_level = static_cast<spdlog::level::level_enum>(level);
      log_impl_pointer->set_level(log_level);
    }
    return log_impl_pointer;
  }

  template <size_t N, size_t M, typename L>
  static auto make_adapter(char const (&log_name)[N], char const (&prefix)[M], L const level)
  {
    auto const filename = stlw::concat(prefix, log_name, ".log");
    auto logger = make_spdlog_logger(log_name, level, filename.data(), 23, 59);
    return impl::make_log_adapter(std::move(logger));
  }

  template <size_t N>
  inline static auto make_default_log_group(char const (&prefix)[N])
  {
    // clang-format off
    return impl::make_log_group(
      make_adapter("trace", prefix, impl::log_level::trace),
      make_adapter("debug", prefix, impl::log_level::debug),
      make_adapter("info", prefix, impl::log_level::info),
      make_adapter("warn", prefix, impl::log_level::warn),
      make_adapter("error", prefix, impl::log_level::error)
    );
    // clang-format on
  }

  template <size_t N>
  inline static auto make_aggregate_logger(char const (&prefix)[N])
  {
    static char constexpr LOG_NAME[] = "aggregate";
    auto const log_file_path = stlw::concat(prefix, LOG_NAME, ".log");
    std::array<spdlog::sink_ptr, 2> const sinks = {
        std::make_unique<spdlog::sinks::stderr_sink_st>(),
        std::make_unique<spdlog::sinks::daily_file_sink_st>(log_file_path.data(), 23, 59)};
    auto shared_logger = std::make_unique<spdlog::logger>(LOG_NAME, begin(sinks), end(sinks));
    shared_logger->set_level(spdlog::level::trace);
    return impl::make_log_adapter(std::move(shared_logger));
  }

public:
  static auto make_default_logger(char const *)
  {
    // 1. Construct an instance of the default log group.
    // 2. Construct an instance of a logger that writes all log levels to a shared file.
    static char const prefix[] = "build-system/bin/";
    auto log_group = make_default_log_group(prefix);
    auto ad = make_aggregate_logger(prefix);
    return impl::make_log_writer(std::move(log_group), std::move(ad));
  }
};

} // ns stlw
