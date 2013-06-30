#ifndef PPJ_AST_H
#define PPJ_AST_H

#include <cstdlib>

#include <iostream>
#include <vector>
#include <set>
#include <map>
using namespace std;

#include "symtable.h"
#include "function.h"

struct ASTree {
  int line;
  string token;
  string source;

  ASTree *dad;
  vector<ASTree*> adj;

  int type;
  Function *fun;
  bool l_value;
  int type_inherited;
  bool is_loop;

  int array_size;
  vector<int> *arg_types;
  string arg_name;

  ASTree() {
    dad = NULL;

    type = 0;
    fun = NULL;
    l_value = false;
    type_inherited = 0;
    is_loop = false;

    arg_types = NULL;
  }

  virtual void dfs(SymTable *sym) = 0;

  void check(bool test, string msg);
};

ASTree *makeProduction(string token, string children);

#endif
