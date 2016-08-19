#pragma once

// http://stackoverflow.com/a/2670919/562174
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define CURRENT_LINE_AS_CSTRING STRINGIZE(__LINE__)
