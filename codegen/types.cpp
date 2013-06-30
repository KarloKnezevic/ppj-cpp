#include "types.h"

bool is_convertible_implicit(int a, int b) {
  if (a & M_FUNCTION) return false;
  if (b & M_FUNCTION) return false;

  if ((a & M_ARRAY) != (b & M_ARRAY)) return false;
  if (a & M_ARRAY) {
    return a == b || (a | M_CONST) == b;
  }

  a |= M_CONST;
  b |= M_CONST;
  if (a == b) return true;

  if (a & M_CHAR) {
    a = a ^ M_CHAR ^ M_INT;
  }
  return a == b;
}

bool is_convertible_explicit(int a, int b) {
  if (is_convertible_implicit(a, b)) return true;
  return (a & M_INT) && is_convertible_implicit(a ^ M_INT ^ M_CHAR, b);
}
