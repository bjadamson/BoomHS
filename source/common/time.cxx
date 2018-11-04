#include <common/time.hpp>
#include <assert.h>

using namespace common;

namespace
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// seconds -> T
auto
seconds_to_minutes(int64_t const seconds)
{
  return seconds / 60;
}

auto
seconds_to_hours(int64_t const seconds)
{
  return seconds / (60 * 60);
}

auto
seconds_to_days(int64_t const seconds)
{
  return seconds / (60 * 60 * 24);
}

auto
seconds_to_weeks(int64_t const seconds)
{
  return seconds / (60 * 60 * 24 * 7);
}

auto
seconds_to_months(int64_t const seconds)
{
  return seconds / (60 * 60 * 24 * 7 * 4);
}

auto
seconds_to_years(int64_t const seconds)
{
  return seconds / (60 * 60 * 24 * 7 * 4 * 12);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// T -> seconds
int64_t
minutes_to_seconds(int const minutes)
{
  return minutes * 60;
}

auto
hours_to_seconds(int const hours)
{
  auto const minutes = 60 * hours;
  return minutes_to_seconds(minutes);
}

auto
days_to_seconds(int const days)
{
  auto const hours = 24 * days;
  return hours_to_seconds(hours);
}

auto
weeks_to_seconds(int const weeks)
{
  auto const days = 7 * weeks;
  return days_to_seconds(days);
}

auto
months_to_seconds(int const months)
{
  auto const weeks = 4 * months;
  return weeks_to_seconds(weeks);
}

auto
years_to_seconds(int const years)
{
  auto const months = 12 * years;
  return months_to_seconds(months);
}

} // namespace

namespace common
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TimeConversions
ticks_t
TimeConversions::ticks_to_millis(ticks_t const t, freq_t const freq)
{
  assert(t != 0.0);
  return t * 1000.0 / freq;
}

ticks_t
TimeConversions::millis_to_ticks(ticks_t const t, freq_t const freq)
{
  return t / 1000.0 * freq;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ticks_t
TimeConversions::millis_to_seconds(ticks_t const m)
{
  return m * 0.001;
}

ticks_t
TimeConversions::seconds_to_millis(ticks_t const s)
{
  return s * 1000.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Time
int64_t
Time::elapsed() const
{
  return elapsed_ - elapsed_time_last_reset_;
}

int64_t
Time::offset() const
{
  return seconds_ + minutes_to_seconds(minutes_) + hours_to_seconds(hours_) +
         days_to_seconds(days_) + weeks_to_seconds(weeks_) + months_to_seconds(months_) +
         years_to_seconds(years_);
}

int64_t
Time::total_seconds() const
{
  return elapsed() + offset();
}

void
Time::reset()
{
  auto const difference = elapsed_ - elapsed_time_last_reset_;
  elapsed_time_last_reset_ += difference;
}

int
Time::seconds() const
{
  auto const seconds = total_seconds();
  return seconds % 60;
}

void
Time::add_seconds(int const v)
{
  seconds_ += v;
  if (seconds_ > 59) {
    add_minutes(1);
    seconds_ = 0;
  }
}

void
Time::set_seconds(int const v)
{
  seconds_ = v;
}

int
Time::minutes() const
{
  auto const seconds = total_seconds();
  return seconds_to_minutes(seconds) % 60;
}

void
Time::add_minutes(int const v)
{
  minutes_ += v;
  if (minutes_ > 59) {
    add_hours(1);
    minutes_ = 0;
  }
}

void
Time::set_minutes(int const v)
{
  minutes_ = v;
}

int
Time::hours() const
{
  auto const seconds = total_seconds();
  return seconds_to_hours(seconds) % 24;
}

void
Time::add_hours(int const v)
{
  hours_ += v;
  if (hours_ > 23) {
    add_days(1);
    hours_ = 0;
  }
}

void
Time::set_hours(int const v)
{
  hours_ = v;
}

int
Time::days() const
{
  auto const seconds = total_seconds();
  return seconds_to_days(seconds) % 7;
}

void
Time::set_days(int const v)
{
  days_ = v;
}

void
Time::add_days(int const v)
{
  days_ += v;
  if (days_ > 30) {
    add_weeks(1);
    days_ = 0;
  }
}

int
Time::weeks() const
{
  auto const seconds = total_seconds();
  return seconds_to_weeks(seconds) % 4;
}

void
Time::add_weeks(int const v)
{
  weeks_ += v;
  if (weeks_ > 4) {
    add_months(1);
    weeks_ = 0;
  }
}

void
Time::set_weeks(int const v)
{
  weeks_ = v;
}

int
Time::months() const
{
  auto const seconds = total_seconds();
  return seconds_to_months(seconds) % 12;
}

void
Time::add_months(int const v)
{
  months_ += v;
  if (months_ > 12) {
    add_years(1);
    months_ = 0;
  }
}

void
Time::set_months(int const v)
{
  months_ = v;
}

int
Time::years() const
{
  auto const seconds = total_seconds();
  return seconds_to_years(seconds) % 365;
}

void
Time::add_years(int const v)
{
  years_ += v;
}

void
Time::set_years(int const v)
{
  years_ = v;
}

void
Time::set_speed(int const speed)
{
  speed_ = speed;
}

void
Time::update(int64_t const since_beginning_seconds)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winteger-overflow"
  __int128_t const DIVISOR = 365 * 12 * 4 * 7 * 24 * 60 * 60;
#pragma GCC diagnostic pop

  elapsed_ = (speed() * since_beginning_seconds) % DIVISOR;
}

Result<std::string, char const*>
Time::get_time_now()
{
  auto const now       = std::chrono::system_clock::now();
  auto const now_timet = std::chrono::system_clock::to_time_t(now);
  auto const now_tm    = *std::localtime(&now_timet);

  char buff[70];
  if (!std::strftime(buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S%z", &now_tm)) {
    return Err("Could not read current time into buffer.");
  }

  return Ok(std::string{buff});
}

} // namespace common
