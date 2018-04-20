#pragma once
#include <stlw/auto_resource.hpp>
#include <stlw/compiler.hpp>
#include <stlw/type_macros.hpp>
#include <extlibs/fmt.hpp>

namespace stlw::impl
{

enum class LogLevel {
  trace = 0,
  debug,
  info,
  warn,
  error,
  MAX,
};

// Flag controlling which fmt policy the logger uses.
enum class FormatPolicy {
  none = 0,
  sprintf,
  format
};

// Log adapter class.
//
// Adapts an underlying logger to the standard logging interface.
//
// It is assumed the value passed in has unique_ptr semantics.
class LogAdapter
{
  LoggerPointer logger_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(LogAdapter);
  explicit LogAdapter(LoggerPointer &&logger)
      : logger_(MOVE(logger))
  {
  }

#define DEFINE_LOG_ADAPTER_METHOD(FN_NAME)                                                         \
  template <typename... Params>                                                                    \
  auto&                                                                                            \
  macro_callmeonly_##FN_NAME(FormatPolicy const policy, Params &&... p)                            \
  {                                                                                                \
    switch(policy) {                                                                               \
      case FormatPolicy::none:                                                                     \
        {                                                                                          \
          auto const passthrough = [](auto const msg, auto &&...) { return msg; };                 \
          this->logger_->FN_NAME(passthrough(FORWARD(p)));                                         \
        }                                                                                          \
        break;                                                                                     \
      case FormatPolicy::sprintf:                                                                  \
        this->logger_->FN_NAME(fmt::sprintf(FORWARD(p)));                                          \
        break;                                                                                     \
      case FormatPolicy::format:                                                                   \
        this->logger_->FN_NAME(fmt::format(FORWARD(p)));                                           \
        break;                                                                                     \
      default:                                                                                     \
        std::abort();                                                                              \
        break;                                                                                     \
    }                                                                                              \
    return *this;                                                                                  \
  }

  DEFINE_LOG_ADAPTER_METHOD(trace)
  DEFINE_LOG_ADAPTER_METHOD(debug)
  DEFINE_LOG_ADAPTER_METHOD(info)
  DEFINE_LOG_ADAPTER_METHOD(warn)
  DEFINE_LOG_ADAPTER_METHOD(error)

#undef DEFINE_LOG_ADAPTER_METHOD

  void set_level(spdlog::level::level_enum const level)
  {
    logger_->set_level(level);
  }

  void destroy()
  {
    flush();
  }

  void flush()
  {
    logger_->flush();
  }
};

using LogFlusher = AutoResource<LogAdapter>;

#define DEFINE_LOGWRITER_FN(FN_NAME)                                                               \
  template <typename... Params>                                                                    \
  auto&                                                                                            \
  macro_callmeonly_##FN_NAME(FormatPolicy const policy, Params &&... p)                            \
  {                                                                                                \
    this->flusher_.resource().macro_callmeonly_##FN_NAME(policy, FORWARD(p));                      \
    return *this;                                                                                  \
  }

class LogWriter
{
  LogFlusher flusher_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(LogWriter);
  explicit LogWriter(LogFlusher &&lf)
      : flusher_(MOVE(lf))
  {
  }

  DEFINE_LOGWRITER_FN(trace)
  DEFINE_LOGWRITER_FN(debug)
  DEFINE_LOGWRITER_FN(info)
  DEFINE_LOGWRITER_FN(warn)
  DEFINE_LOGWRITER_FN(error)

  void flush()
  {
    flusher_.resource().flush();
  }
};

#undef LogWriter_DEFINE_FN

} // ns stlw::impl
