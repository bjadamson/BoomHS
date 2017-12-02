#pragma once
#include <extlibs/spdlog.hpp>
#include <memory>
#include <stlw/compiler_macros.hpp>
#include <stlw/type_macros.hpp>

namespace stlw
{

namespace impl {
class log_writer;
} // ns impl
using Logger = ::stlw::impl::log_writer;
using LoggerPointer = std::unique_ptr<spdlog::logger>;

namespace impl
{

enum class log_level {
  trace = 0,
  debug,
  info,
  warn,
  error,
  MAX,
};

// Log adapter class.
//
// Adapts an underlying logger to the standard logging interface.
//
// It is assumed the value passed in has unique_ptr semantics.
class log_adapter
{
  LoggerPointer p_logger_;
  NO_COPY(log_adapter)
public:

  explicit log_adapter(LoggerPointer &&l)
      : p_logger_(MOVE(l))
  {
  }
  log_adapter(log_adapter &&other)
      : p_logger_(MOVE(other.p_logger_))
  {
  }

  log_adapter &operator=(log_adapter &&) = delete;

#define DEFINE_LOG_ADAPTER_METHOD(FN_NAME)                                                         \
  template <typename... Params>                                                                    \
  auto& FN_NAME(Params &&... p)                                                                    \
  {                                                                                                \
    this->p_logger_->FN_NAME(std::forward<Params>(p)...);                                          \
    return *this;                                                                                  \
  }

  DEFINE_LOG_ADAPTER_METHOD(trace)
  DEFINE_LOG_ADAPTER_METHOD(debug)
  DEFINE_LOG_ADAPTER_METHOD(info)
  DEFINE_LOG_ADAPTER_METHOD(warn)
  DEFINE_LOG_ADAPTER_METHOD(error)

  template <typename... Params>
  auto& log_nothing(Params &&...) const { return *this;}

  friend log_adapter make_log_adapter(LoggerPointer &&);
};

class log_group
{
  NO_COPY(log_group)

  log_adapter trace_;
  log_adapter debug_;
  log_adapter info_;
  log_adapter warn_;
  log_adapter error_;

public:
  explicit log_group(log_adapter &&t, log_adapter &&d, log_adapter &&i, log_adapter &&w, log_adapter &&e)
      : trace_(MOVE(t))
      , debug_(MOVE(d))
      , info_(MOVE(i))
      , warn_(MOVE(w))
      , error_(MOVE(e))
  {
  }

  log_group(log_group &&other)
      : trace_(MOVE(other.trace_))
      , debug_(MOVE(other.debug_))
      , info_(MOVE(other.info_))
      , warn_(MOVE(other.warn_))
      , error_(MOVE(other.error_))
  {
  }

#define DEFINE_LOG_GROUP_METHOD(FN_NAME)                                                           \
  template <typename... Params>                                                                    \
  auto &FN_NAME(Params &&... p)                                                                    \
  {                                                                                                \
    this->FN_NAME##_.FN_NAME(std::forward<Params>(p)...);                                          \
    return *this;                                                                                  \
  }

  DEFINE_LOG_GROUP_METHOD(trace)
  DEFINE_LOG_GROUP_METHOD(debug)
  DEFINE_LOG_GROUP_METHOD(info)
  DEFINE_LOG_GROUP_METHOD(warn)
  DEFINE_LOG_GROUP_METHOD(error)

  template <typename... Params>
  auto& log_nothing(Params &&...) const { return *this;}

  log_group &operator=(log_group &&) = delete;
};

#define LOG_WRITER_DEFINE_FN(FN_NAME)                                                              \
  template <typename... Params>                                                                    \
  auto &FN_NAME(Params &&... )                                                                     \
  {                                                                                                \
    return *this;                                                                                  \
  }

#define LOG_WRITER_DEFINE_FN_TEMP(FN_NAME)                                                         \
  template <typename... Params>                                                                    \
  auto &FN_NAME(Params &&... p)                                                                    \
  {                                                                                                \
    this->group_.FN_NAME(std::forward<Params>(p)...);                                              \
    this->shared_.FN_NAME(std::forward<Params>(p)...);                                             \
    return *this;                                                                                  \
  }


class log_writer
{
  log_group group_;
  log_adapter shared_;

  NO_COPY(log_writer)
  log_writer &operator=(log_writer &&) = delete;

public:
  explicit log_writer(log_group &&g, log_adapter &&s)
      : group_(MOVE(g))
      , shared_(MOVE(s))
  {
  }

  log_writer(log_writer &&other)
      : group_(MOVE(other.group_))
      , shared_(MOVE(other.shared_))
  {
  }

  LOG_WRITER_DEFINE_FN(trace)
  LOG_WRITER_DEFINE_FN(debug)
  LOG_WRITER_DEFINE_FN(info)
  LOG_WRITER_DEFINE_FN_TEMP(warn)
  LOG_WRITER_DEFINE_FN_TEMP(error)

  LOG_WRITER_DEFINE_FN_TEMP(log_nothing)
};

} // ns impl
} // ns stlw
