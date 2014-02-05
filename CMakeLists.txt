cmake_minimum_required (VERSION 2.6)
project (reflektor)

# Default build config
if (NOT CMAKE_BUILD_TYPE)
#  message(STATUS "No build type selected, default to Release")
#  set(CMAKE_BUILD_TYPE "Release")
	set(CMAKE_BUILD_TYPE "Debug")
endif()

# Some compiler flags
SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11" )
set( CMAKE_C_FLAGS_DEBUG "-O2 -Wall -Wextra -pedantic -g -Wno-unused-parameter" )
set( CMAKE_C_FLAGS_RELEASE "-O2 -Wall -fomit-frame-pointer -DNDEBUG" )
set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g" )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

#find_package(PortAudio)
#
#if( NOT PORTAUDIO_FOUND )
#	message(FATAL_ERROR "PortAudio can't be found.")
#endif()


find_package(FFTW)

################################
# Header Files
################################
#include_directories(${reggie_SOURCE_DIR}/include/)

################################
# Source Files
################################
# set(reflektor_SOURCES 


add_executable(reflektor src/reflektor.c)
target_link_libraries(reflektor portaudio)
target_link_libraries(reflektor m)
target_link_libraries(reflektor fftw3f)
