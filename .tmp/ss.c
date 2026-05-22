#include <stdint.h>
int64_t sum_squares(int64_t n){int64_t s=0;for(int64_t i=1;i<=n;i++)s+=i*i;return s;}
int64_t bench(int p){int64_t b=0;for(int i=0;i<p;i++)b+=sum_squares(100000);return b;}
