#ifndef PPJ_AUTOMATON_H
#define PPJ_AUTOMATON_H

#include <string>
#include <vector>

using namespace std;

struct State {
  long bio;
  vector<char> how;
  vector<State*> next;

  void addTrans(char c, State *s) {
    how.push_back(c);
    next.push_back(s);
  }

  static State *make();
};

extern int numStates;
extern State memo[];

struct Automaton {
  State *s, *t;

  Automaton(State *s = 0, State *t = 0) {
    this->s = s;
    this->t = t;
  }

  char *maxMatch(char *c);
};

#endif
