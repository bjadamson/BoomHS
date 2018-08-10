#pragma once
#include <common/result.hpp>
#include <common/type_macros.hpp>
#include <string>

namespace boomhs
{

class Time
{
  // seconds
  int64_t elapsed_                 = 0;
  int64_t elapsed_time_last_reset_ = 0;

  int seconds_ = 0, minutes_ = 0, hours_ = 0;
  int days_ = 0, weeks_ = 0, months_ = 0, years_ = 0;

  int speed_ = 1;

  int64_t elapsed() const;
  int64_t offset() const;
  int64_t total_seconds() const;

public:
  Time() = default;
  void reset();

  int  seconds() const;
  void add_seconds(int);
  void set_seconds(int);

  int  minutes() const;
  void add_minutes(int);
  void set_minutes(int);

  int  hours() const;
  void add_hours(int);
  void set_hours(int);

  int  days() const;
  void add_days(int);
  void set_days(int);

  int  weeks() const;
  void add_weeks(int);
  void set_weeks(int);

  int  months() const;
  void add_months(int);
  void set_months(int);

  int  years() const;
  void add_years(int);
  void set_years(int);

  auto speed() const { return speed_; }
  void set_speed(int);

  void update(int64_t);
  MOVE_CONSTRUCTIBLE_ONLY(Time);

  static Result<std::string, char const*> get_time_now();
};

} // namespace boomhs
