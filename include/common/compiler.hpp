#pragma once
#include <assert.h>

// Define the symbol DEBUG_BUILD for debug builds, using the strategy from here:
// https://stackoverflow.com/a/8594122
//
#ifndef NDEBUG
#define DEBUG_BUILD
#define FOR_DEBUG_ONLY(fn) fn()
#else
//
// RELEASE build
#undef DEBUG_BUILD
// In Release mode, NOOP
#define FOR_DEBUG_ONLY(fn)
#endif

// http://stackoverflow.com/a/2670919/562174
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define CURRENT_LINE_AS_CSTRING STRINGIZE(__LINE__)
