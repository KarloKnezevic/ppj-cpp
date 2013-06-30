#ifndef PPJ_TYPES_H
#define PPJ_TYPES_H

enum {
  M_INT = 1,
  M_CHAR = 2,
  M_VOID = 4,
  M_CONST = 8,
  M_ARRAY = 16,
  M_FUNCTION = 32,
};

bool is_convertible_implicit(int a, int b);
bool is_convertible_explicit(int a, int b);

#endif
