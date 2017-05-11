#ifndef _PID_TOOLS_H
#define _PID_TOOLS_H


#ifdef __GNUC__

  #define ALWAYS_INLINE __attribute__((always_inline))

  #define likely(x)    __builtin_expect(!!(x),1)
  #define unlikely(x)  __builtin_expect(!!(x),0)
  #define __hot __attribute__((hot))

#else

  #define ALWAYS_INLINE
  #define likely(x)   (x)
  #define unlikely(x) (x)
  #define __hot

#endif

#ifndef SHARED_LIB
  #define STATIC_EXE_ONLY static
  #define INLINE_EXE_ONLY inline
  #define STATIC_INLINE_EXE_ONLY static

  #define ALWAYS_INLINE_EXE_ONLY ALWAYS_INLINE
#else
  #define STATIC_EXE_ONLY
  #define INLINE_EXE_ONLY
  #define STATIC_INLINE_EXE_ONLY
  #define ALWAYS_INLINE_EXE_ONLY
#endif

const volatile char *PID_TOOLS_VERSION = "2.0.0";

#endif
