#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#include <stdbool.h>

#define INVERT      "not %[tmp1], %[tmp1]\n\t"
#define DONT_INVERT ""

#define GENERATE_FETCH_AND_OP(type, size, opname, insn, invert, cop) \
  type __sync_fetch_and_ ## opname ## _ ## size (type *p, type v)    \
  {                                                             \
    unsigned long aligned_addr = ((unsigned long) p) & ~3UL;    \
    int shift = (((unsigned long) p) & 3) * 8;                  \
    unsigned mask = ((1U << ((sizeof v) * 8)) - 1) << shift;    \
    unsigned old, tmp1, tmp2;                                   \
                                                                \
    asm volatile ("1:\n\t"                                      \
          "lr.w.aq %[old], %[mem]\n\t"                          \
          #insn " %[tmp1], %[old], %[value]\n\t"                \
          invert                                                \
          "and %[tmp1], %[tmp1], %[mask]\n\t"                   \
          "and %[tmp2], %[old], %[not_mask]\n\t"                \
          "or %[tmp2], %[tmp2], %[tmp1]\n\t"                    \
          "sc.w.rl %[tmp1], %[tmp2], %[mem]\n\t"                \
          "bnez %[tmp1], 1b"                                    \
          : [old] "=&r" (old),                                  \
            [mem] "+A" (*(volatile unsigned*) aligned_addr),    \
            [tmp1] "=&r" (tmp1),                                \
            [tmp2] "=&r" (tmp2)                                 \
          : [value] "r" (((unsigned) v) << shift),              \
            [mask] "r" (mask),                                  \
            [not_mask] "r" (~mask));                            \
                                                                \
    return (type) (old >> shift);                               \
  }                                                             \
                                                                \
  type __sync_ ## opname ## _and_fetch_ ## size (type *p, type v) \
  {                                                             \
    type o = __sync_fetch_and_ ## opname ## _ ## size (p, v);   \
    return cop;       \
  }                                                             \
  type __atomic_fetch_ ## opname ## _ ## size (type *p, type v, int memmodel) \
  {                                                             \
    return __sync_fetch_and_ ## opname ## _ ## size (p, v);   \
  }

#define GENERATE_COMPARE_AND_SWAP(type, size)                   \
  type __sync_val_compare_and_swap_ ## size (type *p, type o, type n) \
  {                                                             \
    unsigned long aligned_addr = ((unsigned long) p) & ~3UL;    \
    int shift = (((unsigned long) p) & 3) * 8;                  \
    unsigned mask = ((1U << ((sizeof o) * 8)) - 1) << shift;    \
    unsigned old, tmp1;                                         \
                                                                \
    asm volatile ("1:\n\t"                                      \
          "lr.w.aq %[old], %[mem]\n\t"                          \
          "and %[tmp1], %[old], %[mask]\n\t"                    \
          "bne %[tmp1], %[o], 1f\n\t"                           \
          "and %[tmp1], %[old], %[not_mask]\n\t"                \
          "or %[tmp1], %[tmp1], %[n]\n\t"                       \
          "sc.w.rl %[tmp1], %[tmp1], %[mem]\n\t"                \
          "bnez %[tmp1], 1b\n\t"                                \
          "1:"                                                  \
          : [old] "=&r" (old),                                  \
            [mem] "+A" (*(volatile unsigned*) aligned_addr),    \
            [tmp1] "=&r" (tmp1)                                 \
          : [o] "r" ((((unsigned) o) << shift) & mask),         \
            [n] "r" ((((unsigned) n) << shift) & mask),         \
            [mask] "r" (mask),                                  \
            [not_mask] "r" (~mask));                            \
                                                                \
    return (type) (old >> shift);                               \
  }                                                             \
  bool __sync_bool_compare_and_swap_ ## size (type *p, type o, type n) \
  {                                                             \
    return __sync_val_compare_and_swap(p, o, n) == o;           \
  }                                                            \
  bool __atomic_compare_exchange_ ## size (type *p, type *o, type n,  bool weak, int success, int fail) \
  {                                                             \
    type old = __sync_val_compare_and_swap(p, *o, n);          \
    if ( old == *o ) {                                          \
        return true;                                            \
    } else {                                                    \
        *o = old;                                               \
        return false;                                           \
    }                                                           \
  }                                                             \
  type __atomic_exchange_ ## size (type *p, type v, int memmodel) \
  {                                                             \
    type old;                                                   \
    while (1) {                                                 \
        old = *p;                                               \
        if (__sync_val_compare_and_swap(p, old, v) == old) {    \
            return old;                                         \
        }                                                       \
    }                                                           \
  }                                                             \

#define GENERATE_ALL(type, size)                                \
  GENERATE_FETCH_AND_OP(type, size, add, add, DONT_INVERT, o + v) \
  GENERATE_FETCH_AND_OP(type, size, sub, sub, DONT_INVERT, o - v) \
  GENERATE_FETCH_AND_OP(type, size, and, and, DONT_INVERT, o & v) \
  GENERATE_FETCH_AND_OP(type, size, xor, xor, DONT_INVERT, o ^ v) \
  GENERATE_FETCH_AND_OP(type, size, or, or, DONT_INVERT, o | v)   \
  GENERATE_FETCH_AND_OP(type, size, nand, and, INVERT, ~(o & v))  \
  GENERATE_COMPARE_AND_SWAP(type, size)

GENERATE_ALL(unsigned char, 1)
GENERATE_ALL(unsigned short, 2)

