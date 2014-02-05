# Try to find the PortAudio library
#
# Once done this will define:
#
#  PORTAUDIO_FOUND - system has the PortAudio library
#  PORTAUDIO_INCLUDE_DIR - the PortAudio include directory
#  PORTAUDIO_LIBRARY - the libraries needed to use PortAudio
#  PORTAUDIO_MAINLOOP_LIBRARY - the libraries needed to use PulsAudio Mailoop
#
# Copyright (c) 2008, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2009, Marcus Hufgard, <Marcus.Hufgard@hufgard.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (NOT PORTAUDIO_MINIMUM_VERSION)
  set(PORTAUDIO_MINIMUM_VERSION "0.9.9")
endif (NOT PORTAUDIO_MINIMUM_VERSION)

if (PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY AND PORTAUDIO_MAINLOOP_LIBRARY)
   # Already in cache, be silent
   set(PORTAUDIO_FIND_QUIETLY TRUE)
endif (PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY AND PORTAUDIO_MAINLOOP_LIBRARY)

if (NOT WIN32)
   include(FindPkgConfig)
   pkg_check_modules(PC_PORTAUDIO libpulse>=${PORTAUDIO_MINIMUM_VERSION})
   pkg_check_modules(PC_PORTAUDIO_MAINLOOP libpulse-mainloop-glib)
endif (NOT WIN32)

FIND_PATH(PORTAUDIO_INCLUDE_DIR pulse/pulseaudio.h
   HINTS
   ${PC_PORTAUDIO_INCLUDEDIR}
   ${PC_PORTAUDIO_INCLUDE_DIRS}
   )

FIND_LIBRARY(PORTAUDIO_LIBRARY NAMES pulse libpulse
   HINTS
   ${PC_PORTAUDIO_LIBDIR}
   ${PC_PORTAUDIO_LIBRARY_DIRS}
   )

FIND_LIBRARY(PORTAUDIO_MAINLOOP_LIBRARY NAMES pulse-mainloop pulse-mainloop-glib libpulse-mainloop-glib
   HINTS
   ${PC_PORTAUDIO_LIBDIR}
   ${PC_PORTAUDIO_LIBRARY_DIRS}
   )

if (PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
   include(MacroEnsureVersion)

   # get PortAudio's version from its version.h, and compare it with our minimum version
   file(STRINGS "${PORTAUDIO_INCLUDE_DIR}/pulse/version.h" pulse_version_h
        REGEX ".*pa_get_headers_version\\(\\).*"
        )
   string(REGEX REPLACE ".*pa_get_headers_version\\(\\)\ \\(\"([0-9]+\\.[0-9]+\\.[0-9]+)\"\\).*" "\\1"
                         PORTAUDIO_VERSION "${pulse_version_h}")
   macro_ensure_version("${PORTAUDIO_MINIMUM_VERSION}" "${PORTAUDIO_VERSION}" PORTAUDIO_FOUND)
else (PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
   set(PORTAUDIO_FOUND FALSE)
endif (PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)

if (PORTAUDIO_FOUND)
   if (NOT PORTAUDIO_FIND_QUIETLY)
      message(STATUS "Found PortAudio: ${PORTAUDIO_LIBRARY}")
      if (PORTAUDIO_MAINLOOP_LIBRARY)
          message(STATUS "Found PortAudio Mainloop: ${PORTAUDIO_MAINLOOP_LIBRARY}")
      else (PULSAUDIO_MAINLOOP_LIBRARY)
          message(STATUS "Could NOT find PortAudio Mainloop Library")
      endif (PORTAUDIO_MAINLOOP_LIBRARY)
   endif (NOT PORTAUDIO_FIND_QUIETLY)
else (PORTAUDIO_FOUND)
   message(STATUS "Could NOT find PortAudio")
endif (PORTAUDIO_FOUND)

mark_as_advanced(PORTAUDIO_INCLUDE_DIR PORTAUDIO_LIBRARY PORTAUDIO_MAINLOOP_LIBRARY)
