# Finds DL library
#
#  Dl_INCLUDE_DIR - where to find bfd.h, etc.
#  Dl_LIBRARIES   - List of libraries when using bfd.
#  Dl_FOUND       - True if bfd found.


if (Dl_INCLUDE_DIR)
  # Already in cache, be silent
  set(Dl_FIND_QUIETLY TRUE)
endif (Dl_INCLUDE_DIR)

find_path(Dl_INCLUDE_DIR bfd.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Dl_NAMES bfd)
find_library(Dl_LIBRARY
  NAMES ${Dl_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Dl_INCLUDE_DIR AND Dl_LIBRARY)
   set(Dl_FOUND TRUE)
   set( Dl_LIBRARIES ${Dl_LIBRARY} )
else (Dl_INCLUDE_DIR AND Dl_LIBRARY)
   set(Dl_FOUND FALSE)
   set(Dl_LIBRARIES)
endif (Dl_INCLUDE_DIR AND Dl_LIBRARY)

if (Dl_FOUND)
   if (NOT Dl_FIND_QUIETLY)
      message(STATUS "Found BFD: ${Dl_LIBRARY}")
   endif (NOT Dl_FIND_QUIETLY)
else (Dl_FOUND)
   if (Dl_FIND_REQUIRED)
      message(STATUS "Looked for Dl libraries named ${Dl_NAMES}.")
      message(FATAL_ERROR "Could NOT find Dl library")
   endif (Dl_FIND_REQUIRED)
endif (Dl_FOUND)

mark_as_advanced(
  Dl_LIBRARY
  Dl_INCLUDE_DIR
  )
