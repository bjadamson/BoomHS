#pragma once
#include <extlibs/spdlog.hpp>
#include <memory>

// clang-format off
#undef LOG_TRACE
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR

#ifdef NDEBUG
#define LOG_TRACE_IMPL(...)
#define LOG_DEBUG_IMPL(...)
#define LOG_INFO_IMPL(...)
#define LOG_WARN_IMPL(...)
#define LOG_ERROR_IMPL(...)
#else
#define LOG_TRACE_IMPL(fmtpolicy, ...) logger.macro_callmeonly_trace(fmtpolicy, __VA_ARGS__)
#define LOG_DEBUG_IMPL(fmtpolicy, ...) logger.macro_callmeonly_debug(fmtpolicy, __VA_ARGS__)
#define LOG_INFO_IMPL(fmtpolicy, ...)  logger.macro_callmeonly_info(fmtpolicy, __VA_ARGS__)
#define LOG_WARN_IMPL(fmtpolicy, ...)  logger.macro_callmeonly_warn(fmtpolicy, __VA_ARGS__)
#define LOG_ERROR_IMPL(fmtpolicy, ...) logger.macro_callmeonly_error(fmtpolicy, __VA_ARGS__)
#endif

#define LOG_TRACE(...) LOG_TRACE_IMPL(::stlw::impl::FormatPolicy::none, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_DEBUG_IMPL(::stlw::impl::FormatPolicy::none, __VA_ARGS__)
#define LOG_INFO(...)  LOG_INFO_IMPL(::stlw::impl::FormatPolicy::none, __VA_ARGS__)
#define LOG_WARN(...)  LOG_WARN_IMPL(::stlw::impl::FormatPolicy::none, __VA_ARGS__)
#define LOG_ERROR(...) LOG_ERROR_IMPL(::stlw::impl::FormatPolicy::none, __VA_ARGS__)

#define LOG_TRACE_SPRINTF(...) LOG_TRACE_IMPL(::stlw::impl::FormatPolicy::sprintf, __VA_ARGS__)
#define LOG_DEBUG_SPRINTF(...) LOG_DEBUG_IMPL(::stlw::impl::FormatPolicy::sprintf, __VA_ARGS__)
#define LOG_INFO_SPRINTF(...)  LOG_INFO_IMPL(::stlw::impl::FormatPolicy::sprintf, __VA_ARGS__)
#define LOG_WARN_SPRINTF(...)  LOG_WARN_IMPL(::stlw::impl::FormatPolicy::sprintf, __VA_ARGS__)
#define LOG_ERROR_SPRINTF(...) LOG_ERROR_IMPL(::stlw::impl::FormatPolicy::sprintf, __VA_ARGS__)

#define LOG_TRACE_FORMAT(...) LOG_TRACE_IMPL(::stlw::impl::FormatPolicy::format, __VA_ARGS__)
#define LOG_DEBUG_FORMAT(...) LOG_DEBUG_IMPL(::stlw::impl::FormatPolicy::format, __VA_ARGS__)
#define LOG_INFO_FORMAT(...)  LOG_INFO_IMPL(::stlw::impl::FormatPolicy::format, __VA_ARGS__)
#define LOG_WARN_FORMAT(...)  LOG_WARN_IMPL(::stlw::impl::FormatPolicy::format, __VA_ARGS__)
#define LOG_ERROR_FORMAT(...) LOG_ERROR_IMPL(::stlw::impl::FormatPolicy::format, __VA_ARGS__)
// clang-format on

namespace stlw
{
using LoggerPointer = std::unique_ptr<spdlog::logger>;
} // ns stlw

#include <stlw/impl/log_impl.hpp>

namespace stlw
{
using Logger = ::stlw::impl::LogWriter;

struct LogFactory
{
  LogFactory() = delete;
public:
  static impl::LogWriter make_default(char const *);
};

} // ns stlw
