#pragma once
#include <memory>
#include <stlw/format.hpp>
#include <stlw/type_macros.hpp>

namespace stlw
{

template <typename L>
class logger
{
  L logger_;
  logger(L &&l)
      : logger_(std::move(l))
  {
  }

  NO_COPY(logger)
public:
  MOVE_DEFAULT(logger)

#define LOGGER_IMPL_DEFINE_FORWARDING_FN(FN_NAME)                                                  \
  template <typename... Params>                                                                    \
  logger &FN_NAME(Params &&... p)                                                                  \
  {                                                                                                \
    this->logger_->FN_NAME(std::forward<Params>(p)...);                                            \
    return *this;                                                                                  \
  }

  LOGGER_IMPL_DEFINE_FORWARDING_FN(debug)
  LOGGER_IMPL_DEFINE_FORWARDING_FN(warning)
  LOGGER_IMPL_DEFINE_FORWARDING_FN(info)
  LOGGER_IMPL_DEFINE_FORWARDING_FN(error)

  friend class log_factory;
};

class log_factory
{
  template <typename F>
  static auto create_logger(F &&f) -> logger<F>
  {
    return {std::forward<F>(f)};
  }

public:
  template <typename... Params>
  static decltype(auto) make_logger(char const* log_name, Params &&... p)
  {
    // TODO: pass in a boost::filesystem::path as the directory.
    auto constexpr debug_filename = "debug.log";

    std::array<spdlog::sink_ptr, 2> const sinks = {
        std::make_unique<spdlog::sinks::stdout_sink_st>(),
        std::make_unique<spdlog::sinks::daily_file_sink_st>(std::forward<Params>(p)...)};

    auto log_impl_pointer = std::make_unique<spdlog::logger>(log_name, begin(sinks), end(sinks));
    return create_logger(std::move(log_impl_pointer));
  }
};

} // ns stlw
