#include "regex.h"

Automaton autoOr(Automaton A, Automaton B) {
  A.s->addTrans(0, B.s);
  B.t->addTrans(0, A.t);
  return A;
}

Automaton autoSeq(Automaton A, Automaton B) {
  A.t->addTrans(0, B.s);
  A.t = B.t;
  return A;
}

Automaton autoStar(Automaton A) {
  A.s->addTrans(0, A.t);
  A.t->addTrans(0, A.s);
  return A;
}

Automaton autoChar(char c) {
  State *s = State::make();
  State *t = State::make();
  s->addTrans(c, t);
  return Automaton(s, t);
}

vector<char> regex;
vector<bool> ctrl;

bool isOpen (int i) { return ctrl[i] && regex[i] == '('; }
bool isClose(int i) { return ctrl[i] && regex[i] == ')'; }
bool isStar (int i) { return ctrl[i] && regex[i] == '*'; }
bool isOr   (int i) { return ctrl[i] && regex[i] == '|'; }

Automaton build(int lo, int hi) {
  int firstClose = -1;
  int level = 0;

  for (int i = lo; i <= hi; ++i) {
    if (isOpen(i)) ++level;
    if (isClose(i)) --level;
    if (isClose(i) && firstClose == -1 && level == 0) firstClose = i;

    if (isOr(i) && level == 0)
      return autoOr(build(lo, i - 1), build(i + 1, hi));
  }

  int mid = lo+1;
  if (isOpen(lo)) mid = firstClose + 1;

  bool star = false;
  if (mid <= hi && isStar(mid)) {
    ++mid;
    star = true;
  }

  Automaton A = isOpen(lo) ? build(lo + 1, firstClose - 1) : autoChar(regex[lo]);
  if (star) A = autoStar(A);

  if (mid > hi) return A;
  return autoSeq(A, build(mid, hi));
}

Automaton regexAutomaton(string str) {
  regex.clear();
  ctrl.clear();

  bool esc = false;
  for (string::iterator it = str.begin(); it != str.end(); ++it) {
    if (esc) {
      if      (*it == '_') regex.push_back(' ');
      else if (*it == 'n') regex.push_back('\n');
      else if (*it == 't') regex.push_back('\t');
      else                 regex.push_back(*it);

      ctrl.push_back(false);
      esc = false;

    } else if (*it == '\\') {
      esc = true;

    } else {
      regex.push_back(*it == '$' ? 0 : *it);
      ctrl.push_back(*it == '(' || *it == ')' || *it == '*' || *it == '|');
    }
  }

  return build(0, regex.size()-1);
}
