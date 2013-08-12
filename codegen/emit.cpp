#include <cstdio>
#include <cstdarg>

#include <sstream>

#include "emit.h"

string l_global = makeLabel();

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

string makeLabel() {
  static int n = 0;
  stringstream ss;
  ss << "L_" << ++n;

  string label = ss.str();
  return label;
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
  emit("ADC R0, 0, R6");
}

void emitFunctionStart() {
  emit("PUSH R5");
  emit("MOVE SP, R5");
}

void emitFunctionEnd() {
  emit("MOVE R5, SP");
  emit("POP R5");
  emit("RET");
}
