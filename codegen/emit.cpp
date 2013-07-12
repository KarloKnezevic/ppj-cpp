#include <cstdio>
#include <cstdarg>

#include "emit.h"

void emit() {
  printf("\n");
}

void emit(const char *format, ...) {
  va_list args;
  va_start(args, format);

  printf("\t");
  vprintf(format, args);
  printf("\n");

  va_end(args);
}

void emitLabel(const char *format, ...) {
  va_list args;
  va_start(args, format);

  vprintf(format, args);
  printf("\n");

  va_end(args);
}

void emitIntToBool() {
  emit("MOVE 0, R0");
  emit("SUB R0, R6, R0");
  emit("ADD R6, R0, R0");
  emit("MOVE 0, R6");
  emit("ADC R6, 0, R6");
}
