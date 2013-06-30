#ifndef PPJ_LANGUAGE_H
#define PPJ_LANGUAGE_H

#include "rule.h"

namespace Lang {
  extern int initState;
  extern vector< vector<Rule> > rules;
  extern vector<string> lexemes;

  extern void init();
}

#endif
