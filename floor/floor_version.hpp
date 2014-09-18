/*
 *  Flo's Open libRary (floor)
 *  Copyright (C) 2004 - 2014 Florian Ziesche
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __FLOOR_VERSION_HPP__
#define __FLOOR_VERSION_HPP__

// floor version and build/compiler info
#include "floor/build_version.hpp"
#include "core/util.hpp"

#define FLOOR_VERSION_STRINGIFY(ver) #ver
#define FLOOR_VERSION_EVAL(ver) FLOOR_VERSION_STRINGIFY(ver)

// <major>.<minor>.<revision><dev_stage>-<build>
#define FLOOR_MAJOR_VERSION 0
#define FLOOR_MINOR_VERSION 1
#define FLOOR_REVISION_VERSION 3
#define FLOOR_DEV_STAGE_VERSION 0xd1
#define FLOOR_DEV_STAGE_VERSION_STR "d1"
// FLOOR_BUILD_VERSION defined in build_version.hpp

#define FLOOR_MAJOR_VERSION_STR FLOOR_VERSION_EVAL(FLOOR_MAJOR_VERSION)
#define FLOOR_MINOR_VERSION_STR FLOOR_VERSION_EVAL(FLOOR_MINOR_VERSION)
#define FLOOR_REVISION_VERSION_STR FLOOR_VERSION_EVAL(FLOOR_REVISION_VERSION)
#define FLOOR_BUILD_VERSION_STR FLOOR_VERSION_EVAL(FLOOR_BUILD_VERSION)

#define FLOOR_COMPAT_VERSION FLOOR_MAJOR_VERSION_STR "." FLOOR_MINOR_VERSION_STR "." FLOOR_REVISION_VERSION_STR
#define FLOOR_FULL_VERSION FLOOR_COMPAT_VERSION FLOOR_DEV_STAGE_VERSION_STR "-" FLOOR_BUILD_VERSION_STR

#define FLOOR_BUILD_TIME __TIME__
#define FLOOR_BUILD_DATE __DATE__

#if defined(FLOOR_DEBUG) || defined(DEBUG)
#define FLOOR_DEBUG_STR " (debug)"
#else
#define FLOOR_DEBUG_STR ""
#endif

#if defined(_MSC_VER)
#define FLOOR_COMPILER "VC++ "+size_t2string(_MSC_VER)
#elif (defined(__GNUC__) && !defined(__llvm__) && !defined(__clang__))
#define FLOOR_COMPILER string("GCC ")+__VERSION__
#elif (defined(__GNUC__) && defined(__llvm__) && !defined(__clang__))
#define FLOOR_COMPILER string("LLVM-GCC ")+__VERSION__
#elif defined(__clang__)
#define FLOOR_COMPILER string("Clang ")+__clang_version__
#else
#define FLOOR_COMPILER "unknown compiler"
#endif

#define FLOOR_LIBCXX_PREFIX " and "
#if defined(_LIBCPP_VERSION)
#define FLOOR_LIBCXX FLOOR_LIBCXX_PREFIX+"libc++ "+size_t2string(_LIBCPP_VERSION)
#elif defined(__GLIBCXX__)
#define FLOOR_LIBCXX FLOOR_LIBCXX_PREFIX+"libstdc++ "+size_t2string(__GLIBCXX__)
#else
#define FLOOR_LIBCXX ""
#endif

#if !defined(FLOOR_IOS)
#define FLOOR_PLATFORM (sizeof(void*) == 4 ? "x86" : (sizeof(void*) == 8 ? "x64" : "unknown"))
#else
#define FLOOR_PLATFORM (sizeof(void*) == 4 ? "ARM32" : (sizeof(void*) == 8 ? "ARM64" : "unknown"))
#endif

#define FLOOR_VERSION_STRING (string("floor ")+FLOOR_PLATFORM+FLOOR_DEBUG_STR \
" v"+(FLOOR_FULL_VERSION)+\
" ("+FLOOR_BUILD_DATE+" "+FLOOR_BUILD_TIME+") built with "+string(FLOOR_COMPILER+FLOOR_LIBCXX))

#define FLOOR_SOURCE_URL "https://github.com/a2flo/floor"


// compiler checks:
// msvc check
#if defined(_MSC_VER)
#if (_MSC_VER <= 1800)
#error "Sorry, but you need MSVC 13.0+ (VS 2014+) to compile floor"
#endif

// clang check
#elif defined(__clang__)
#if !defined(__clang_major__) || !defined(__clang_minor__) || (__clang_major__ < 3) || (__clang_major__ == 3 && __clang_minor__ < 5)
#error "Sorry, but you need Clang 3.5+ to compile floor"
#endif

// gcc check
#elif defined(__GNUC__)
#if (__GNUC__ < 5)
#error "Sorry, but you need GCC 5.x+ to compile floor"
#endif

// just fall through ...
#else
#endif

// library checks:
#include "core/platform.hpp"
#include <openssl/opensslv.h>

#if (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 1101)
#error "You need to install libc++ 1101+ to compile floor"
#endif

#if !SDL_VERSION_ATLEAST(2, 0, 2)
#error "You need to install SDL 2.0.2+ to compile floor"
#endif

#if (OPENSSL_VERSION_NUMBER < 0x1000105fL)
#error "You need to install OpenSSL 1.0.1e+ to compile floor"
#endif

#endif
