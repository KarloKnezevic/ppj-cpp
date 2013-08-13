#include <cstdio>
#include <iostream>

#include "rule.h"
#include "language.h"

char *readInput(FILE *f) {
  static string str;
  char c;
  while ((c = fgetc(f)) != EOF) str += c;
  return (char*) str.c_str();
}

int main(void) {
  Lang::init();

  char *src = readInput(stdin);
  int state = Lang::initState;
  int line = 1;
  bool ok = true;

  while (*src) {
    char *next = 0;
    Rule *R = 0;

    vector<Rule> &v = Lang::rules[state];
    for (vector<Rule>::iterator it = v.begin(); it != v.end(); ++it) {
      char *pos = it->A.maxMatch(src);
      if (pos > next) {
        next = pos;
        R = &*it;
      }
    }

    if (next == 0) {
      ok = false;
      cerr << line << ": error: unexpected char `" << *src << "`" << endl;
      ++src;
      continue;
    }

    for (vector<Action>::iterator it = R->acts.begin(); it != R->acts.end(); ++it) {
      if (it->type == NEW_LINE)
        ++line;
      else if (it->type == GO_STATE)
        state = it->go;
      else if (it->type == GO_BACK)
        next = src + it->go;
    }

    if (R->lexeme != -1) {
      cout << Lang::lexemes[R->lexeme] << " " << line << " ";
      for (char *c = src; c < next; ++c) cout << *c;
      cout << endl;
    }

    src = next;
  }

  return ok ? 0 : 1;
}
