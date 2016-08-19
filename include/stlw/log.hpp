#pragma once
#include <memory>
#include <extlibs/spdlog.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/compiler_macros.hpp>


// Logging macros
#define LOG_ADAPTER_DEFINE_FN(FN_NAME)                \
template <typename... Params>                         \
auto& FN_NAME(Params &&... p)               \
{                                                     \
  this->logger_->FN_NAME(std::forward<Params>(p)...); \
  return *this;                                       \
}

#define LOG_ADAPTER_DEFINE_DEBUG_FN(FN_NAME)               \
template <typename... Params>                              \
auto& FN_NAME(Params &&... p)                    \
{                                                          \
  SPDLOG_TRACE(this->logger_, std::forward<Params>(p)...); \
  return *this;                                            \
}

namespace stlw
{

enum class log_level
{
  trace = 0,
  debug = 1,
  info = 2,
  warn = 3,
  error = 4,
};

template <typename L>
class log_adapter
{
  L logger_;

  log_adapter(L &&l)
      : logger_(std::move(l))
  {
  }

  NO_COPY(log_adapter)
public:
  log_adapter(log_adapter &&other)
    : logger_(std::move(other.logger_))
  {
  }

  log_adapter& operator=(log_adapter &&) = delete;

#define DEFINE_LOG_ADAPTER_METHOD(FN_NAME)             \
  template <typename... Params>                        \
  auto& FN_NAME(Params &&... p)                        \
  {                                                    \
    this->logger_->FN_NAME(std::forward<Params>(p)...); \
    return *this;                                      \
  }

  DEFINE_LOG_ADAPTER_METHOD(trace)
  DEFINE_LOG_ADAPTER_METHOD(debug)
  DEFINE_LOG_ADAPTER_METHOD(info)
  DEFINE_LOG_ADAPTER_METHOD(warn)
  DEFINE_LOG_ADAPTER_METHOD(error)

  friend class log_factory;
};

template<typename L>
class log_group
{
  NO_COPY(log_group)

  log_adapter<L> trace_;
  log_adapter<L> debug_;
  log_adapter<L> info_;
  log_adapter<L> warn_;
  log_adapter<L> error_;
public:
  explicit log_group(log_adapter<L> &&t,
      log_adapter<L> &&d,
      log_adapter<L> &&i,
      log_adapter<L> &&w,
      log_adapter<L> &&e)
    : trace_(std::move(t))
    , debug_(std::move(d))
    , info_(std::move(i))
    , warn_(std::move(w))
    , error_(std::move(e))
  {
  }

  log_group(log_group &&other)
    : trace_(std::move(other.trace_))
    , debug_(std::move(other.debug_))
    , info_(std::move(other.info_))
    , warn_(std::move(other.warn_))
    , error_(std::move(other.error_))
  {
  }

#define DEFINE_LOG_GROUP_METHOD(FN_NAME)              \
  template <typename... Params>                       \
  log_group& FN_NAME(Params &&... p)                  \
  {                                                   \
    this->FN_NAME##_.FN_NAME(std::forward<Params>(p)...);\
    return *this;                                     \
  }

  DEFINE_LOG_GROUP_METHOD(trace)
  DEFINE_LOG_GROUP_METHOD(debug)
  DEFINE_LOG_GROUP_METHOD(info)
  DEFINE_LOG_GROUP_METHOD(warn)
  DEFINE_LOG_GROUP_METHOD(error)

  log_group& operator=(log_group &&) = delete;
};

#define LOGGER_DEFINE_FN(FN_NAME)                     \
template <typename... Params>                         \
auto& FN_NAME(Params &&... p)                       \
{                                                     \
  this->group_.FN_NAME(std::forward<Params>(p)...);   \
  return *this;                                       \
}

template<typename L>
class log_writer
{
  log_group<L> group_;
  NO_COPY(log_writer)
public:
  explicit log_writer(log_group<L> &&g) : group_(std::move(g)) {}

  log_writer(log_writer &&other)
    : group_(std::move(other.group_))
  {
  }

  LOGGER_DEFINE_FN(trace)
  LOGGER_DEFINE_FN(debug)
  LOGGER_DEFINE_FN(info)
  LOGGER_DEFINE_FN(warn)
  LOGGER_DEFINE_FN(error)
};

class log_factory
{
  template <typename F>
  static auto instantiate_logger(F &&f) -> log_adapter<F>
  {
    return log_adapter<F>{std::forward<F>(f)};
  }

  template <typename... Params>
  static auto make_logger(char const* log_name, log_level const level, Params &&... p)
  {
    std::array<spdlog::sink_ptr, 2> const sinks = {
        std::make_unique<spdlog::sinks::stdout_sink_st>(),
        std::make_unique<spdlog::sinks::daily_file_sink_st>(std::forward<Params>(p)...)};

    auto log_impl_pointer = std::make_unique<spdlog::logger>(log_name, begin(sinks), end(sinks));
    {
      auto const log_level = static_cast<spdlog::level::level_enum>(level);
      log_impl_pointer->set_level(log_level);
    }
    return log_impl_pointer;
    //return instantiate_logger(std::move(log_impl_pointer));
  }

  static auto make_default_log_group(char const* name)
  {
    auto const ml = [](auto const* name, auto const level) {
      return make_logger(name, level, name + std::string{".log"}, "txt", 23, 59);
    };

    using T = decltype(ml(name, log_level::trace));
    return log_group<T>{
        ml("trace", log_level::trace),
        ml("debug", log_level::debug),
        ml("info", log_level::info),
        ml("warn", log_level::warn),
        ml("error", log_level::error)};
  }

  template <typename L>
  static auto make_log_writer(log_group<L> &&group)
  {
    return log_writer<L>{std::move(group)};
  }
public:

  static auto make_default_logger(char const* name)
  {
    auto log_group = make_default_log_group(name);
    return make_log_writer(std::move(log_group));
  }
};

} // ns stlw
