#include <cstdio>
#include <cctype>

#include <iostream>
#include <sstream>
#include <vector>
#include <stack>
using namespace std;

#include "ast.h"
#include "types.h"
#include "symtable.h"

struct Node {
  string def;
  string token;
  vector<Node*> adj;
};

Node *readTree() {
  stack<Node*> stk;

  for (string line; getline(cin, line); ) {
    int offset = 0;
    while (line[offset] == ' ') {
      ++offset;
    }
    line = line.substr(offset);

    while ((int) stk.size() > offset) {
      stk.pop();
    }

    stringstream ss(line);
    string token;
    ss >> token;
    if (token[0] == '<') {
      token = token.substr(1, token.size() - 2);
    }

    Node *node = new Node;
    node->def = line;
    node->token = token;

    if (!stk.empty()) {
      stk.top()->adj.push_back(node);
    }
    stk.push(node);
  }

  while (stk.size() > 1) {
    stk.pop();
  }
  return stk.top();
}

string extractSource(string def) {
  int n = def.size();
  int pos = 0;
  for (int step = 0; step < 2; ++step) {
    while (pos < n && def[pos] != ' ') ++pos;
    if (pos < n) ++pos;
  }
  return def.substr(pos);
}

ASTree *makeAST(Node *node) {
  string children = "";
  for (vector<Node*>::iterator it = node->adj.begin(); it != node->adj.end(); ++it) {
    if (it != node->adj.begin()) {
      children += "__";
    }
    children += (*it)->token;
  }
  ASTree *ast = makeProduction(node->token, children);

  stringstream ss(node->def);
  ss >> ast->token >> ast->line;
  ast->source = extractSource(node->def);

  for (vector<Node*>::iterator it = node->adj.begin(); it != node->adj.end(); ++it) {
    ASTree *child = makeAST(*it);
    child->dad = ast;
    ast->adj.push_back(child);
  }

  return ast;
}

int main() {
  Node *root = readTree();
  ASTree *ast = makeAST(root);

  SymTable *sym = new SymTable(NULL);
  ast->dfs(sym);

  const string ERR_MAIN = "main";
  const string ERR_FUN = "funkcija";

  ASTree *main_f = fun_def.get(ERR_MAIN);
  ast->check(main_f, ERR_MAIN);
  ast->check(main_f->fun->type == M_INT, ERR_MAIN);
  ast->check(main_f->fun->args.empty(), ERR_MAIN);

  set< pair<string, Function*> >::iterator it;
  for (it = fun_declared.begin(); it != fun_declared.end(); ++it) {
    string name = it->first;
    Function *fun = it->second;

    ASTree *f = fun_def.get(name);
    ast->check(f, ERR_FUN);
    ast->check(*f->fun == *fun, ERR_FUN);
  }

  return 0;
}