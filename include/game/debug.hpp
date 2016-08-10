#pragma once
#include <stlw/format.hpp>
#include <stlw/result.hpp>

#define FORMAT_STRERR(ARG1) stlw::make_error(boost::str(ARG1))
