const int MAX = 20;
int prime[20];

int main(void) {
  int i, j;
  int n = 7;

  for (i = 2; i < MAX; ++i)
    prime[i] = 1;
  for (i = 2; i*i < MAX; ++i)
    if (prime[i])
      for (j = i*i; j < MAX; j = j+i)
        prime[j] = 0;

  for (i = 0; i < MAX; ++i)
    if (prime[i])
      if (--n == 0)
        return i;

  return 0;
}
