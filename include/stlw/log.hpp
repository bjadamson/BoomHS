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
  static std::unique_ptr<spdlog::logger>
  make_spdlog_logger(char const *log_name, impl::log_level const level, Params &&... p)
  {
    std::array<spdlog::sink_ptr, 2> const sinks = {
        std::make_unique<spdlog::sinks::stdout_sink_st>(),
        std::make_unique<spdlog::sinks::daily_file_sink_st>(std::forward<Params>(p)...)};

    auto log_impl_pointer = std::make_unique<spdlog::logger>(log_name, begin(sinks), end(sinks));
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
    return impl::log_group<T>{
        make_adapter("trace", impl::log_level::trace),
        make_adapter("debug", impl::log_level::debug), make_adapter("info", impl::log_level::info),
        make_adapter("warn", impl::log_level::warn), make_adapter("error", impl::log_level::error)};
  }

  template <typename L>
  inline static auto make_log_writer(impl::log_group<L> &&group)
  {
    return impl::log_writer<L>{std::move(group)};
  }

public:
  static auto make_default_logger(char const *name)
  {
    auto log_group = make_default_log_group(name);
    return make_log_writer(std::move(log_group));
  }
};

} // ns stlw
