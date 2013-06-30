#ifndef PPJ_FUNCTION_H
#define PPJ_FUNCTION_H

#include <vector>
using namespace std;

struct Function {
  int type;
  vector<int> args;

  friend bool operator == (const Function &A, const Function &B) {
    return A.type == B.type && A.args == B.args;
  }
};

#endif
