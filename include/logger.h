#pragma once

#ifdef _FIRE_DEBUG_

  #include <stdio.h>
  #include <stdarg.h>
  #include <string.h>

  #define LOGGER(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

#else

  #define LOGGER(...) ((void)0)

#endif
