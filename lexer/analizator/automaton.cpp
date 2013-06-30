#include <queue>

#include "automaton.h"

int numStates = 0;
State memo[100000];

State *State::make() {
  State *s = &memo[numStates++];
  return s;
}

char *Automaton::maxMatch(char *a) {
  vector<State*> curr;
  curr.push_back(this->s);

  char *ret = 0;
  for (; !curr.empty(); ++a) {
    static long cookie = 1;
    ++cookie;

    queue<State*> Q;
    for (vector<State*>::iterator s = curr.begin(); s != curr.end(); ++s) {
      (*s)->bio = cookie;
      Q.push(*s);
    }

    for (; !Q.empty(); Q.pop()) {
      State *s = Q.front();

      int n = s->how.size();
      for (int i = 0; i < n; ++i) {
        if (s->how[i] != 0) continue;

        State *t = s->next[i];
        if (t->bio != cookie) {
          t->bio = cookie;
          curr.push_back(t);
          Q.push(t);
        }
      }
    }

    if (this->t->bio == cookie) ret = a;
    if (*a == 0) break;

    vector<State*> next;
    for (vector<State*>::iterator s = curr.begin(); s != curr.end(); ++s) {
      int n = (*s)->how.size();
      for (int i = 0; i < n; ++i)
        if ((*s)->how[i] == *a)
          next.push_back((*s)->next[i]);
    }
    curr = next;
  }
  return ret;
}
