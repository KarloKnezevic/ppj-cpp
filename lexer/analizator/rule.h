#ifndef PPJ_RULE_H
#define PPJ_RULE_H

#include <vector>

#include "automaton.h"

enum {
  GO_BACK,
  NEW_LINE,
  GO_STATE
};

struct Action {
  int type;
  int go;
  Action(int type, int go = -1) {
    this->type = type;
    this->go = go;
  }
};

struct Rule {
  int lexeme;
  Automaton A;
  vector<Action> acts;

  Rule() {}
  Rule(int lexeme, Automaton A, vector<Action> acts) {
    this->lexeme = lexeme;
    this->A = A;
    this->acts = acts;
  }
};

#endif
