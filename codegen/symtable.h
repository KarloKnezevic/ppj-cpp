#ifndef PPJ_SYMTABLE_H
#define PPJ_SYMTABLE_H

#include <string>
#include <stack>
#include <map>
#include <set>
using namespace std;

#include "function.h"

struct ASTree;

struct SymTable {
  SymTable *dad;
  map<string, ASTree*> table;

  int stack_top;
  int base_pointer;

  stack<string> stk_continue;
  stack<string> stk_break;

  Function *fun;

  SymTable(SymTable *dad);
  SymTable *extend();

  int push();
  int push(int count);

  void addLoop(string l_continue, string l_break);
  void removeLoop();
  string getContinue();
  string getBreak();
  bool insideLoop();

  void put(string s, ASTree *ast);
  ASTree *get(string s);
  ASTree *getLocal(string s);
};

extern SymTable fun_def;
extern set< pair<string, Function*> > fun_declared;

#endif
