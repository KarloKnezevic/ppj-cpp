#include <cstdio>
#include <cstdarg>

#include <sstream>
#include <iostream>

#include "emit.h"

string l_global = makeLabel();

ostringstream code;
static char buffer[1000];

void emit() {
  code << '\n';
}

void emit(const char *format, ...) {
  va_list args;
  va_start(args, format);

  vsprintf(buffer, format, args);
  code << '\t' << buffer << '\n';

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

  vsprintf(buffer, format, args);
  code << buffer << '\n';

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

void outputCode() {
  cout << code.str();
}
