#pragma once
#include <memory>
#include <extlibs/spdlog.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/compiler_macros.hpp>

namespace stlw
{

enum class log_level
{
  trace = 0,
  debug,
  info,
  warn,
  error,
  MAX,
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

#define DEFINE_LOG_ADAPTER_METHOD(FN_NAME)              \
  template <typename... Params>                         \
  auto& FN_NAME(Params &&... p)                         \
  {                                                     \
    this->logger_->FN_NAME(std::forward<Params>(p)...); \
    return *this;                                       \
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

  L trace_;
  L debug_;
  L info_;
  L warn_;
  L error_;
public:
  explicit log_group(L &&t, L &&d, L &&i, L &&w, L &&e)
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

#define DEFINE_LOG_GROUP_METHOD(FN_NAME)                  \
  template <typename... Params>                           \
  auto& FN_NAME(Params &&... p)                           \
  {                                                       \
    this->FN_NAME##_.FN_NAME(std::forward<Params>(p)...); \
    return *this;                                         \
  }

  DEFINE_LOG_GROUP_METHOD(trace)
  DEFINE_LOG_GROUP_METHOD(debug)
  DEFINE_LOG_GROUP_METHOD(info)
  DEFINE_LOG_GROUP_METHOD(warn)
  DEFINE_LOG_GROUP_METHOD(error)

  log_group& operator=(log_group &&) = delete;
};

#define LOG_WRITER_DEFINE_FN(FN_NAME)                 \
template <typename... Params>                         \
auto& FN_NAME(Params &&... p)                         \
{                                                     \
  this->group_.FN_NAME(std::forward<Params>(p)...);   \
  return *this;                                       \
}

template<typename L>
class log_writer
{
  log_group<L> group_;

  NO_COPY(log_writer)
  log_writer& operator=(log_writer &&) = delete;
public:
  explicit log_writer(log_group<L> &&g) : group_(std::move(g)) {}

  log_writer(log_writer &&other)
    : group_(std::move(other.group_))
  {
  }

  LOG_WRITER_DEFINE_FN(trace)
  LOG_WRITER_DEFINE_FN(debug)
  LOG_WRITER_DEFINE_FN(info)
  LOG_WRITER_DEFINE_FN(warn)
  LOG_WRITER_DEFINE_FN(error)
};

class log_factory
{
  template <typename... Params>
  static std::unique_ptr<spdlog::logger>
  make_spdlog_logger(char const* log_name, log_level const level, Params &&... p)
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
    //return log_adapter<decltype(log_impl_pointer)>(std::move(log_impl_pointer));
  }

  template <typename L>
  static log_adapter<L> make_log_adapter(L &&logger)
  {
    return log_adapter<L>{std::move(logger)};
  }

  inline static auto make_default_log_group(char const* group_name)
  {
      auto const ml = [](auto const* name, auto const level) -> log_adapter<std::unique_ptr<spdlog::logger>> {
      return make_log_adapter(make_spdlog_logger(name, level, name + std::string{".log"}, "txt", 23, 59));
    };

    // TODO: this is weird , maybe we need a log_level::max index instead?
    using T = decltype(ml(group_name, log_level::trace));
    return log_group<T>{
        ml("trace", log_level::trace),
        ml("debug", log_level::debug),
        ml("info", log_level::info),
        ml("warn", log_level::warn),
        ml("error", log_level::error)};
  }

  template <typename L>
  inline static auto make_log_writer(log_group<L> &&group)
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
