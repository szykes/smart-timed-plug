#include "framework.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void own_log(const char *func, unsigned int line, const char *lvl, const char *format, ...) {
  if (strcmp(lvl, "TEST") != 0) {
    printf("%s:%d - %s: ", func, line, lvl);
  }

  va_list arglist;
  va_start(arglist, format);
  vprintf(format, arglist);
  va_end(arglist);

  printf("\n");
}
