#pragma once
#include <boost/format.hpp>
#include <stlw/type_macros.hpp>

namespace stlw
{

DEFINE_WRAPPER_FUNCTION(to_string, boost::str);
DEFINE_WRAPPER_FUNCTION(format, boost::format);

} // ns stlw
