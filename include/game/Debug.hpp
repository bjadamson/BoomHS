#pragma once
#include <boost/expected/expected.hpp>
#include <boost/format.hpp>

#define FORMAT_STRERR(ARG1) \
  boost::make_unexpected(boost::str(ARG1))
