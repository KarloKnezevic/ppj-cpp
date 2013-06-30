#ifndef PPJ_PARSE_H
#define PPJ_PARSE_H

#include <string>
#include <vector>
using namespace std;

bool parseInt(string s, int &res);

bool parseChar(string s, int &res);

bool parseString(string s, vector<int> &res);

#endif
