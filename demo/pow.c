int pow(int a, int b) {
  int res = 1;
  for (; b > 0; b = b/2) {
    if (b & 1) res = res * a;
    a = a * a;
  }
  return res;
}

int main(void) {
  return pow(2, 30);
}
