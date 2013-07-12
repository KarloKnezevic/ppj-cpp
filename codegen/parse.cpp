#include <cstdio>

#include "parse.h"

int digitValue(char c) {
  if (isdigit(c))
    return c - '0';
  else
    return tolower(c) - 'a' + 10;
}

bool parseInt(string s, int &res) {
  if (s.length() == 0) return false;

  int base = 10;
  if (s.size() >= 2 && s[0] == '0' && tolower(s[1]) == 'x') {
    base = 16;
    s = s.substr(2);
  } else if (s[0] == '0') {
    base = 8;
    s = s.substr(1);
  }

  long long num = 0;
  for (string::iterator it = s.begin(); it != s.end(); ++it) {
    num = num * base + digitValue(*it);

    // TODO: constant -2^31 (minimum int) fails this test
    res = num;
    if (res != num) return false;
  }
  res = num;

  return true;
}

char getEscaped(char c) {
  const int N = 6;
  const char from[N] = {'t', 'n', '0', '\'', '\"', '\\'};
  const char to[N] = {'\t', '\n', '\0', '\'', '\"', '\\'};

  for (int i = 0; i < N; ++i) {
    if (c == from[i]) {
      return to[i];
    }
  }
  return -1;
}

bool parseChar(string s, int &res) {
  if (s.size() < 2) return false;
  s = s.substr(1, s.size() - 2);

  if (s.size() == 1) {
    res = s[0];
    return res != '\\';
  } else if (s.size() == 2) {
    res = getEscaped(s[1]);
    return s[0] == '\\' && res != -1;
  } else {
    return false;
  }
}

bool parseString(string s, vector<int> &res) {
  if (s.size() < 2) return false;
  s = s.substr(1, s.size() - 2);

  res = vector<int>();
  bool esc = false;

  for (string::iterator it = s.begin(); it != s.end(); ++it) {
    if (esc) {
      esc = false;
      int c = getEscaped(*it);
      if (c == -1) return false;
      res.push_back(c);
    } else if (*it == '\\') {
      esc = true;
    } else if (*it == '\"') {
      return false;
    } else {
      res.push_back(*it);
    }
  }
  res.push_back(0);

  return !esc;
}
