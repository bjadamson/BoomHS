#pragma once
#include <stlw/result.hpp>
#include <boost/format.hpp>

#define FORMAT_STRERR(ARG1) \
  stlw::make_error(boost::str(ARG1))
