#include <stdint.h>

typedef struct {
  int32_t a;
  int32_t b;
  int32_t c;
} ThreeI32;

typedef struct {
  uint8_t a;
  uint8_t b;
  uint8_t c;
} Odd3;

int32_t mettle_struct_abi_c_sum_three(ThreeI32 t) {
  return t.a + t.b + t.c;
}

ThreeI32 mettle_struct_abi_c_make_three(int32_t a, int32_t b, int32_t c) {
  ThreeI32 t;
  t.a = a;
  t.b = b;
  t.c = c;
  return t;
}

int32_t mettle_struct_abi_c_sum_odd3(Odd3 o) {
  return (int32_t)o.a + (int32_t)o.b + (int32_t)o.c;
}

Odd3 mettle_struct_abi_c_make_odd3(uint8_t a, uint8_t b, uint8_t c) {
  Odd3 o;
  o.a = a;
  o.b = b;
  o.c = c;
  return o;
}
