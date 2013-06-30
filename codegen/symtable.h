#ifndef PPJ_SYMTABLE_H
#define PPJ_SYMTABLE_H

#include <string>
#include <map>
#include <set>
using namespace std;

#include "function.h"

struct ASTree;

struct SymTable {
  SymTable *dad;
  map<string, ASTree*> table;

  SymTable(SymTable *dad);
  SymTable *extend();

  void put(string s, ASTree *ast);
  ASTree *get(string s);
  ASTree *getLocal(string s);
};

extern SymTable fun_def;
extern set< pair<string, Function*> > fun_declared;

#endif
