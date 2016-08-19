#pragma once
#include <memory>
#include <stlw/compiler_macros.hpp>
#include <stlw/type_macros.hpp>

#include <stlw/impl/log_impl.hpp>

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

  inline static auto make_default_log_group(char const *group_name)
  {
    auto const make_adapter = [](auto const *name, auto const level) {
      auto logger = make_spdlog_logger(name, level, name + std::string{".log"}, "txt", 23, 59);
      return impl::make_log_adapter(std::move(logger));
    };

    using T = decltype(make_adapter(group_name, impl::log_level::MAX));
    // clang-format off
    return impl::log_group<T>{
      make_adapter("trace", impl::log_level::trace),
      make_adapter("debug", impl::log_level::debug),
      make_adapter("info", impl::log_level::info),
      make_adapter("warn", impl::log_level::warn),
      make_adapter("error", impl::log_level::error)
    };
    // clang-format on
  }

  inline static auto make_aggregate_logger()
  {
    static auto constexpr LOG_NAME = "aggregate";
    std::array<spdlog::sink_ptr, 2> const sinks = {
      std::make_unique<spdlog::sinks::stdout_sink_st>(),
      std::make_unique<spdlog::sinks::daily_file_sink_st>("aggregate.log", "txt", 23, 59)
    };
    auto shared_logger = std::make_unique<spdlog::logger>(LOG_NAME, begin(sinks), end(sinks));
    shared_logger->set_level(spdlog::level::trace);
    return impl::make_log_adapter(std::move(shared_logger));
  }

public:
  static auto make_default_logger(char const *name)
  {
    // 1. Construct an instance of the default log group.
    // 2. Construct an instance of a logger that writes all log levels to a shared file.
    auto log_group = make_default_log_group(name);
    auto ad = make_aggregate_logger();
    return impl::make_log_writer(std::move(log_group), std::move(ad));
  }
};

} // ns stlw
