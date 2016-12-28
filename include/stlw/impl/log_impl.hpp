#pragma once
#include <extlibs/spdlog.hpp>
#include <memory>
#include <stlw/compiler_macros.hpp>
#include <stlw/type_macros.hpp>

namespace stlw
{
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
template <typename L>
class log_adapter
{
  L p_logger_;

  log_adapter(L &&l)
      : p_logger_(std::move(l))
  {
  }

  NO_COPY(log_adapter)
public:
  log_adapter(log_adapter &&other)
      : p_logger_(std::move(other.p_logger_))
  {
  }

  log_adapter &operator=(log_adapter &&) = delete;

#define DEFINE_LOG_ADAPTER_METHOD(FN_NAME)                                                         \
  template <typename... Params>                                                                    \
  auto &FN_NAME(Params &&... p)                                                                    \
  {                                                                                                \
    this->p_logger_->FN_NAME(std::forward<Params>(p)...);                                          \
    return *this;                                                                                  \
  }

  DEFINE_LOG_ADAPTER_METHOD(trace)
  DEFINE_LOG_ADAPTER_METHOD(debug)
  DEFINE_LOG_ADAPTER_METHOD(info)
  DEFINE_LOG_ADAPTER_METHOD(warn)
  DEFINE_LOG_ADAPTER_METHOD(error)

  template <typename OL>
  friend log_adapter<OL> make_log_adapter(OL &&);
};

template <typename L>
log_adapter<L>
make_log_adapter(L &&logger)
{
  return log_adapter<L>{std::move(logger)};
}

template <typename L>
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

  log_group &operator=(log_group &&) = delete;
};

template <typename P>
auto
make_log_group(P &&a, P &&b, P &&c, P &&d, P &&e)
{
  return log_group<P>(std::move(a), std::move(b), std::move(c), std::move(d), std::move(e));
}

#define LOG_WRITER_DEFINE_FN(FN_NAME)                                                              \
  template <typename... Params>                                                                    \
  auto &FN_NAME(Params &&... p)                                                                    \
  {                                                                                                \
    this->group_.FN_NAME(std::forward<Params>(p)...);                                              \
    this->shared_.FN_NAME(std::forward<Params>(p)...);                                             \
    return *this;                                                                                  \
  }


template <typename L, typename M>
class log_writer
{
  log_group<L> group_;
  log_adapter<M> shared_;

  NO_COPY(log_writer)
  log_writer &operator=(log_writer &&) = delete;

public:
  explicit log_writer(log_group<L> &&g, log_adapter<M> &&s)
      : group_(std::move(g))
      , shared_(std::move(s))
  {
  }

  log_writer(log_writer &&other)
      : group_(std::move(other.group_))
      , shared_(std::move(other.shared_))
  {
  }

  LOG_WRITER_DEFINE_FN(trace)
  LOG_WRITER_DEFINE_FN(debug)
  LOG_WRITER_DEFINE_FN(info)
  LOG_WRITER_DEFINE_FN(warn)
  LOG_WRITER_DEFINE_FN(error)
};

template <typename L, typename M>
inline auto
make_log_writer(impl::log_group<L> &&group, impl::log_adapter<M> &&m)
{
  return log_writer<L, M>{std::move(group), std::move(m)};
}

} // ns impl
} // ns stlw
