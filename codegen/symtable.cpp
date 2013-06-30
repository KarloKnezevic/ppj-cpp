#include "symtable.h"

SymTable::SymTable(SymTable *dad) {
  this->dad = dad;
}

SymTable *SymTable::extend() {
  return new SymTable(this);
}

void SymTable::put(string s, ASTree *ast) {
  table[s] = ast;
}

ASTree *SymTable::get(string s) {
  if (ASTree *t = getLocal(s)) return t;

  if (dad == NULL) return NULL;
  return dad->get(s);
}

ASTree *SymTable::getLocal(string s) {
  map<string, ASTree*>::iterator it = table.find(s);
  return it == table.end() ? NULL : it->second;
}

SymTable fun_def(NULL);
set< pair<string, Function*> > fun_declared;
