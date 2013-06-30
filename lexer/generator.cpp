#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <map>

#include "analizator/rule.h"
#include "regex.h"

map<string, string> def_regex;
map<string, int> id_state;
map<string, int> id_lexeme;

vector<string> states;
vector<string> lexemes;
vector< vector<Rule> > rules;

string line;

void initRegexDefs() {
  for (;;) {
    getline(cin, line);
    if (line[0] == '%') break;

    size_t x = line.find('}');
    string name = line.substr(1, x - 1);
    string def = line.substr(x + 2);

    def_regex[name] = def;
  }
}

void initStates() {
  line = line.substr(3);

  stringstream ss(line);
  for (string s; ss >> s;) {
    id_state[s] = states.size();
    states.push_back(s);
  }
}

void initLexemes() {
  getline(cin, line);
  line = line.substr(3);

  stringstream ss(line);
  for (string s; ss >> s;) {
    id_lexeme[s] = lexemes.size();
    lexemes.push_back(s);
  }
}

string substituteDefs(string regex) {
  string ret = "";
  for (string::iterator it = regex.begin(); it != regex.end(); ++it) {
    if (*it == '\\') {
      ret += *it;
      ++it;
      ret += *it;

    } else if (*it == '{') {
      string name = "";
      for (++it; *it != '}'; ++it) name += *it;
      ret += "(" + substituteDefs(def_regex[name]) + ")";

    } else {
      ret += *it;
    }
  }
  return ret;
}

void initRules() {
  rules.resize(states.size());
  for (;;) {
    if(!getline(cin, line)) break;

    size_t x = line.find('>');
    int state = id_state[line.substr(1, x - 1)];
    string regex = line.substr(x + 1);

    regex = substituteDefs(regex);
    Automaton A = regexAutomaton(regex);

    getline(cin, line);
    getline(cin, line);
    int lexeme = line == "-" ? -1 : id_lexeme[line];

    vector<Action> acts;
    for (;;) {
      getline(cin, line);
      if (line[0] == '}') break;

      if (line[0] == 'U') { // UDJI_U_STANJE
        int go = id_state[line.substr(14)];
        acts.push_back(Action(GO_STATE, go));

      } else if (line[0] == 'N') { // NOVI_REDAK
        acts.push_back(Action(NEW_LINE));

      } else { // VRATI_SE
        int go = atoi(line.substr(9).c_str());
        acts.push_back(Action(GO_BACK, go));
      }
    }

    rules[state].push_back(Rule(lexeme, A, acts));
  }
}

void genCode() {
  FILE *f = fopen("analizator/language.cpp", "w");

  fprintf(f, "#include \"language.h\"\n");
  fprintf(f, "namespace Lang {\n");
  fprintf(f, "int initState;\n");
  fprintf(f, "vector< vector<Rule> > rules;\n");
  fprintf(f, "vector<string> lexemes;\n");
  fprintf(f, "void init() {\n");

  fprintf(f, "initState = 0;\n");
  fprintf(f, "rules.resize(%ld);\n", rules.size());

  for (vector<string>::iterator s = lexemes.begin(); s != lexemes.end(); ++s)
    fprintf(f, "lexemes.push_back(\"%s\");\n", s->c_str());

  fprintf(f, "numStates = %d;\n", numStates);
  for (int i = 0; i < numStates; ++i) {
    for (vector<char>::iterator x = memo[i].how.begin(); x != memo[i].how.end(); ++x)
      fprintf(f, "memo[%d].how.push_back((char) %d);\n", i, (int) *x);
    for (vector<State*>::iterator x = memo[i].next.begin(); x != memo[i].next.end(); ++x)
      fprintf(f, "memo[%d].next.push_back(memo + %ld);\n", i, *x - memo);
  }

  for (vector< vector<Rule> >::iterator v = rules.begin(); v != rules.end(); ++v) {
    for (vector<Rule>::iterator r = v->begin(); r != v->end(); ++r) {
      fprintf(f, "{ Rule R;\n");
      fprintf(f, "R.lexeme = %d;\n", r->lexeme);
      fprintf(f, "R.A.s = &memo[%ld];\n", r->A.s - memo);
      fprintf(f, "R.A.t = &memo[%ld];\n", r->A.t - memo);

      for (vector<Action>::iterator a = r->acts.begin(); a != r->acts.end(); ++a)
        fprintf(f, "R.acts.push_back(Action(%d, %d));\n", a->type, a->go);

      fprintf(f, "rules[%ld].push_back(R); }\n", v - rules.begin());
    }
  }

  fprintf(f, "}\n");
  fprintf(f, "}\n");

  fclose(f);
}

int main() {
  initRegexDefs();
  initStates();
  initLexemes();
  initRules();
  genCode();
  return 0;
}
