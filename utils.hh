#ifndef UTILS_HH
#define UTILS_HH

#include <string>
#include <stdexcept>
#include <cstdarg>

typedef unsigned long long ull;

static void die(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
  exit(0);
}

#endif

// vim: et:sw=2

