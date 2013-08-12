#include "symtable.h"

SymTable::SymTable(SymTable *dad) {
  this->dad = dad;
  if (dad) {
    stack_top = dad->stack_top;
    base_pointer = dad->base_pointer;
    fun = dad->fun;
  } else {
    stack_top = base_pointer = 0;
    fun = NULL;
  }
}

SymTable *SymTable::extend() {
  return new SymTable(this);
}

int SymTable::push() {
  return push(1);
}

int SymTable::push(int count) {
  stack_top += 4 * count;
  return stack_top;
}

void SymTable::addLoop(string l_continue, string l_break) {
  stk_continue.push(l_continue);
  stk_break.push(l_break);
}

void SymTable::removeLoop() {
  if (stk_continue.empty()) {
    dad->removeLoop();
  } else {
    stk_continue.pop();
    stk_break.pop();
  }
}

string SymTable::getContinue() {
  if (stk_continue.empty()) {
    return dad->getContinue();
  } else {
    return stk_continue.top();
  }
}

string SymTable::getBreak() {
  if (stk_break.empty()) {
    return dad->getBreak();
  } else {
    return stk_break.top();
  }
}

bool SymTable::insideLoop() {
  return !stk_continue.empty() || dad->insideLoop();
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
