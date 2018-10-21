/*
 * pid_tools - Copyright (c) 2016, 2017, 2018 Timothy Savannah All Rights Reserved
 *   Licensed under terms of GNU General Purpose License (GPL) version 2
 */
#ifndef _PID_TOOLS_H
#define _PID_TOOLS_H


#ifdef __GNUC__

  #define ALWAYS_INLINE __attribute__((always_inline))

  #define likely(x)    __builtin_expect(!!(x),1)
  #define unlikely(x)  __builtin_expect(!!(x),0)
  #define __hot __attribute__((hot))
  #define MAYBE_UNUSED __attribute__((unused))

  #define ALIGN_4  __attribute__ ((aligned(4)))
  #define ALIGN_8  __attribute__ ((aligned(8)))
  #define ALIGN_16  __attribute__ ((aligned(16)))
  #define ALIGN_32 __attribute__ ((aligned(32)))

  #define builtin_ceil(_x) ( __builtin_ceil((_x)) )

#else

  #define ALWAYS_INLINE
  #define likely(x)   (x)
  #define unlikely(x) (x)
  #define __hot
  #define MAYBE_UNUSED
  
  #define ALIGN_4
  #define ALIGN_8
  #define ALIGN_16
  #define ALIGN_32

  #define builtin_ceil(_x) ( ((float)(_x)) - ((int(_x))) < 1e-6 ? ( int((_x)) ) : (int((float)(_x)) + 1) )

#endif

#ifndef SHARED_LIB
  #define STATIC_EXE_ONLY static
  #define INLINE_EXE_ONLY inline
  #define STATIC_INLINE_EXE_ONLY static

  #define STATIC_SHARED_ONLY static

  #define ALWAYS_INLINE_EXE_ONLY ALWAYS_INLINE
#else
  #define STATIC_EXE_ONLY
  #define INLINE_EXE_ONLY

  #define STATIC_SHARED_ONLY

  #define STATIC_INLINE_EXE_ONLY
  #define ALWAYS_INLINE_EXE_ONLY
#endif

STATIC_SHARED_ONLY MAYBE_UNUSED const volatile char *PID_TOOLS_VERSION = "5.0.1";
STATIC_SHARED_ONLY MAYBE_UNUSED const volatile char *PID_TOOLS_COPYRIGHT = "Copyright (c) 2018 Timothy Savannah All Rights Reserved, licensed under GNU General Purpose License version 2";

#endif
