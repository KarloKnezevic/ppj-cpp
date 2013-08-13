#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>
using namespace std;

enum ActionType {
  SHIFT,
  REDUCE,
  ACCEPT,
  ERROR,
};

struct Action {
  ActionType type;
  string left;
  int num_right;

  Action(ActionType type = ERROR, string left = "", int num_right = 0) {
    this->type = type;
    this->left = left;
    this->num_right = num_right;
  }
};

struct Node {
  string str;
  vector<Node*> adj;

  Node(string str) {
    this->str = str;
  }
};

void traverse(Node *x, string space = "") {
  cout << space << x->str << endl;
  for (vector<Node*>::iterator it = x->adj.begin(); it != x->adj.end(); ++it)
    traverse(*it, space + " ");
}

vector<string> symbols;
set<string> syncs;

vector< map<string, int> > TGoto;
vector< map<string, Action> > TAction;

string getToken(string line) {
  stringstream ss(line);
  string s;
  ss >> s;
  return s;
}

string getLineNumber(string line) {
  stringstream ss(line);
  string s;
  ss >> s >> s;
  return s;
}

string getOriginal(string line) {
  int i = 0;
  while (!isdigit(line[i])) ++i;
  while ( isdigit(line[i])) ++i;
  return line.substr(i+1);
}

bool runParser() {
  vector<string> lines;
  for (string line; getline(cin, line); lines.push_back(line));
  lines.push_back("#");

  stack<Node*> tree;
  stack<int> stk;
  stk.push(0);
  bool ok = true;

  for (vector<string>::iterator it = lines.begin(); it != lines.end(); ) {
    string token = getToken(*it);
    int state = stk.top();
    int succ = TGoto[state][token];
    Action a = TAction[state][token];

    if (a.type == ERROR) {
      ok = false;

      stringstream ss(*it);
      cerr << getLineNumber(*it) << ": error at `" << getOriginal(*it) << "`" << endl;
      cerr << "received: " << token << endl;
      cerr << "expected one of:";
      for (vector<string>::iterator s = symbols.begin(); s != symbols.end(); ++s)
        if ((*s)[0] != '<' && TAction[state][*s].type != ERROR)
          cerr << " " << *s;
      cerr << endl;

      for (; it != lines.end(); ++it) {
        if (syncs.count(getToken(*it))) break;
      }
      if (it == lines.end()) exit(1);

      string token = getToken(*it);
      for (;;) {
        if (stk.empty()) exit(1);
        if (TAction[stk.top()][token].type != ERROR) break;
        stk.pop();
        tree.pop();
      }

    } else if (a.type == SHIFT) {
      tree.push(new Node(*it));
      stk.push(succ);
      ++it;

    } else if (a.type == REDUCE) {
      vector<Node*> v;
      for (int i = 0; i < a.num_right; ++i) {
        v.push_back(tree.top());
        tree.pop();
        stk.pop();
      }
      reverse(v.begin(), v.end());
      if (a.num_right == 0) v.push_back(new Node("$"));

      Node *n = new Node(a.left);
      n->adj = v;
      tree.push(n);
      stk.push(TGoto[stk.top()][a.left]);

    } else if (a.type == ACCEPT) {
      traverse(tree.top());
      break;
    }
  }

  return ok;
}

int main(void) {
#include "gen.h"

  stringstream ss(data);
  int state;
  string token;
  while (ss >> state >> token) {
    ss >> TGoto[state][token];

    string type;
    ss >> type;
    if (type == "SHIFT") {
      TAction[state][token] = Action(SHIFT);
    } else {
      string left;
      int num_right;
      ss >> left >> num_right;
      TAction[state][token] =
        Action(type=="REDUCE" ? REDUCE : ACCEPT, left, num_right);
    }
  }

  return runParser() ? 0 : 1;
}
