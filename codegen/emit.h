#ifndef PPJ_EMIT_H
#define PPJ_EMIT_H

#include <string>
using namespace std;

// Common use cases for registers:
// R5 = base pointer
// R6 = result, exit code, return value
// SP = stack pointer

extern string l_global;

void emit();
void emit(const char *format, ...);

string makeLabel();
void emitLabel(const char *format, ...);

void emitIntToBool();
void emitFunctionStart();
void emitFunctionEnd();

#endif
