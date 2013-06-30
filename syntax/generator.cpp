#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
using namespace std;

struct Prod {
  string next;
  string left;
  vector<string> right;
  int dot;

  Prod(string left = "") {
    this->left = left;
    this->dot = 0;
  }
};

bool operator < (const Prod &A, const Prod &B) {
  if (A.dot   != B.dot)   return A.dot   < B.dot;
  if (A.left  != B.left)  return A.left  < B.left;
  if (A.next  != B.next)  return A.next  < B.next;
  if (A.right != B.right) return A.right < B.right;
  return false;
}

bool operator == (const Prod &A, const Prod &B) {
  return !(A<B) && !(B<A);
}

string init_symbol;
vector<Prod> prods;
set<string> eps_reducible;

ofstream fout("analizator/gen.h");

void genSymbols() {
  fout << "symbols.push_back(\"#\");" << endl;
  fout << "symbols.push_back(\"<>\");" << endl;

  for (int i = 0; i < 2; ++i) {
    string line;
    getline(cin, line);
    stringstream ss(line);

    string s;
    ss >> s;
    while (ss >> s) {
      if (init_symbol == "") init_symbol = s;
      fout << "symbols.push_back(\"" << s << "\");" << endl;
    }
  }
}

void genSyncs() {
  string line;
  getline(cin, line);
  stringstream ss(line);

  string s;
  ss >> s;
  while (ss >> s) fout << "syncs.insert(\"" << s << "\");" << endl;
}

void initProductions() {
  vector<string> lines;
  for (string line; getline(cin, line); lines.push_back(line));
  int L = lines.size();

  for (int i = 0; i < L;) {
    string left = lines[i];

    for (++i; i < L && lines[i][0] == ' '; ++i) {
      Prod P = Prod(left);

      stringstream ss(lines[i]);
      for (string s; ss >> s;)
        if (s != "$")
          P.right.push_back(s);

      prods.push_back(P);
    }
  }
}

void initEpsReducible() {
  for (;;) {
    bool change = false;
    for (vector<Prod>::iterator p = prods.begin(); p != prods.end(); ++p) {
      bool all = true;
      for (vector<string>::iterator s = p->right.begin(); s != p->right.end(); ++s)
        all = all && eps_reducible.count(*s);

      if (all) change = change || eps_reducible.insert(p->left).second;
    }
    if (!change) break;
  }
}

void fillFirst(string sym, set<string> &ret, set<string> &seen) {
  if (seen.count(sym)) return;
  seen.insert(sym);

  if (sym[0] != '<') {
    ret.insert(sym);
    return;
  }

  for (vector<Prod>::iterator p = prods.begin(); p != prods.end(); ++p)
    if (p->left == sym)
      for (vector<string>::iterator s = p->right.begin(); s != p->right.end(); ++s) {
        fillFirst(*s, ret, seen);
        if (eps_reducible.count(*s) == 0) break;
      }
}

set<string> first(string sym) {
  set<string> ret;
  set<string> seen;
  fillFirst(sym, ret, seen);
  return ret;
}

void fillClosure(Prod P, set<Prod> &seen) {
  if (seen.count(P)) return;
  seen.insert(P);

  if (P.dot == (int) P.right.size()) return;
  if (P.right[P.dot][0] != '<') return;

  set<string> next;
  if (P.dot+1 < (int) P.right.size())
    next = first(P.right[P.dot+1]);
  else
    next = first(P.next);

  for (vector<Prod>::iterator p = prods.begin(); p != prods.end(); ++p)
    if (p->left == P.right[P.dot])
      for (set<string>::iterator s = next.begin(); s != next.end(); ++s) {
        Prod Q = *p;
        Q.next = *s;
        fillClosure(Q, seen);
      }
}

map< vector<Prod>, vector<Prod> > memo;

vector<Prod> closure(vector<Prod> V) {
  if (memo.count(V)) return memo[V];

  set<Prod> seen;
  for (vector<Prod>::iterator i = V.begin(); i != V.end(); ++i)
    fillClosure(*i, seen);

  vector<Prod> ret;
  for (set<Prod>::iterator it = seen.begin(); it != seen.end(); ++it)
    ret.push_back(*it);
  return memo[V] = vector<Prod>(seen.begin(), seen.end());
}

vector<Prod> successor(vector<Prod> S, string X) {
  vector<Prod> next;
  for (vector<Prod>::iterator i = S.begin(); i != S.end(); ++i)
    if (i->dot < (int) i->right.size() && i->right[i->dot] == X) {
      Prod P = *i;
      ++P.dot;
      next.push_back(P);
    }
  return closure(next);
}

void makeAction(vector<Prod> S, string X, vector<Prod> succ) {
  if (!succ.empty()) {
    fout << " SHIFT";
    return;
  }

  for (vector<Prod>::iterator p = prods.begin(); p != prods.end(); ++p)
    for (vector<Prod>::iterator i = S.begin(); i != S.end(); ++i) {
      if (i->left == "<>" && i->dot == 1 && X == "#") {
        fout << " ACCEPT " << i->left << " " << i->right.size();
        return;
      }

      if (i->dot == (int) i->right.size())
        if (i->left == p->left && i->right == p->right) {
          fout << " REDUCE " << i->left << " " << i->right.size();
          return;
        }
    }

  cerr << "error: should not happen" << endl;
  exit(2);
}

map<vector<Prod>, int> index;

void fillTables(vector<Prod> st, set< vector<Prod> > &seen) {
  if (seen.count(st)) return;
  seen.insert(st);

  int i_st = seen.size() - 1;
  index[st] = i_st;

  set<string> next;
  for (vector<Prod>::iterator p = st.begin(); p != st.end(); ++p)
    if (p->dot < (int) p->right.size())
      next.insert(p->right[p->dot]);
    else
      next.insert(p->next);

  for (set<string>::iterator s = next.begin(); s != next.end(); ++s) {
    if (next.count(*s)) {
      vector<Prod> succ = successor(st, *s);
      fillTables(succ, seen);
      int i_succ = index[succ];

      fout << " " << i_st << " " << *s << " " << i_succ;
      makeAction(st, *s, succ);
    }
  }
}

void genTables() {
  set< vector<Prod> > seen;
  vector<Prod> start;
  start.push_back(prods[0]);
  start = closure(start);

  fout << "string data = \"";
  fillTables(start, seen);
  fout << "\";" << endl;

  fout << "TGoto.resize(" << index.size() << ");" << endl;
  fout << "TAction.resize(" << index.size() << ");" << endl;
}

int main(void) {
  genSymbols();
  genSyncs();

  Prod P("<>");
  P.next = "#";
  P.right.push_back(init_symbol);
  prods.push_back(P);
  initProductions();

  initEpsReducible();

  genTables();

  return 0;
}
