#include "hash-common.h"

struct swPrimeModMask
{
  uint32_t primeMod;
  uint32_t mask;
};

static const struct swPrimeModMask primeModMask [] =
{
  /*  0 */ {                   1,    0x00000000},     /* For 1 << 0 */

  /*  1 */ {                   2,    0x00000001},
  /*  2 */ {                   3,    0x00000003},
  /*  3 */ {                   7,    0x00000007},
  /*  4 */ {                  13,    0x0000000F},
  /*  5 */ {                  31,    0x0000001F},
  /*  6 */ {                  61,    0x0000003F},
  /*  7 */ {                 127,    0x0000007F},
  /*  8 */ {                 251,    0x000000FF},

  /*  9 */ {                 509,    0x000001FF},
  /* 10 */ {                1021,    0x000003FF},
  /* 11 */ {                2039,    0x000007FF},
  /* 12 */ {                4093,    0x00000FFF},
  /* 13 */ {                8191,    0x00001FFF},
  /* 14 */ {               16381,    0x00003FFF},
  /* 15 */ {               32749,    0x00007FFF},
  /* 16 */ {               65521,    0x0000FFFF},     /* For 1 << 16 */

  /* 17 */ {              131071,    0x0001FFFF},
  /* 18 */ {              262139,    0x0003FFFF},
  /* 19 */ {              524287,    0x0007FFFF},
  /* 20 */ {             1048573,    0x000FFFFF},
  /* 21 */ {             2097143,    0x001FFFFF},
  /* 22 */ {             4194301,    0x003FFFFF},
  /* 23 */ {             8388593,    0x007FFFFF},
  /* 24 */ {            16777213,    0x00FFFFFF},

  /* 25 */ {            33554393,    0x01FFFFFF},
  /* 26 */ {            67108859,    0x03FFFFFF},
  /* 27 */ {           134217689,    0x07FFFFFF},
  /* 28 */ {           268435399,    0x0FFFFFFF},
  /* 29 */ {           536870909,    0x1FFFFFFF},
  /* 30 */ {          1073741789,    0x3FFFFFFF},
  /* 31 */ {          2147483647,    0x7FFFFFFF},     /* For 1 << 31 */
  /* 32 */ {          4294967291,    0xFFFFFFFF}

//  /* 33 */ {          8589934583UL,    0x00000001FFFFFFFFUL},
//  /* 34 */ {         17179869143UL,    0x00000003FFFFFFFFUL},
//  /* 35 */ {         34359738337UL,    0x00000007FFFFFFFFUL},
//  /* 36 */ {         68719476731UL,    0x0000000FFFFFFFFFUL},
//  /* 37 */ {        137438953447UL,    0x0000001FFFFFFFFFUL},
//  /* 38 */ {        274877906899UL,    0x0000003FFFFFFFFFUL},
//  /* 39 */ {        549755813881UL,    0x0000007FFFFFFFFFUL},
//  /* 40 */ {       1099511627689UL,    0x000000FFFFFFFFFFUL},
//
//  /* 41 */ {       2199023255531UL,    0x000001FFFFFFFFFFUL},
//  /* 42 */ {       4398046511093UL,    0x000003FFFFFFFFFFUL},
//  /* 43 */ {       8796093022151UL,    0x000007FFFFFFFFFFUL},
//  /* 44 */ {      17592186044399UL,    0x00000FFFFFFFFFFFUL},
//  /* 45 */ {      35184372088777UL,    0x00001FFFFFFFFFFFUL},
//  /* 46 */ {      70368744177643UL,    0x00003FFFFFFFFFFFUL},
//  /* 47 */ {     140737488355213UL,    0x00007FFFFFFFFFFFUL},
//  /* 48 */ {     281474976710597UL,    0x0000FFFFFFFFFFFFUL},
//
//  /* 49 */ {     562949953421231UL,    0x0001FFFFFFFFFFFFUL},
//  /* 50 */ {    1125899906842597UL,    0x0003FFFFFFFFFFFFUL},
//  /* 51 */ {    2251799813685119UL,    0x0007FFFFFFFFFFFFUL},
//  /* 52 */ {    4503599627370449UL,    0x000FFFFFFFFFFFFFUL},
//  /* 53 */ {    9007199254740881UL,    0x001FFFFFFFFFFFFFUL},
//  /* 54 */ {   18014398509481951UL,    0x003FFFFFFFFFFFFFUL},
//  /* 55 */ {   36028797018963913UL,    0x007FFFFFFFFFFFFFUL},
//  /* 56 */ {   72057594037927931UL,    0x00FFFFFFFFFFFFFFUL},
//
//  /* 57 */ {  144115188075855859UL,    0x01FFFFFFFFFFFFFFUL},
//  /* 58 */ {  288230376151711717UL,    0x03FFFFFFFFFFFFFFUL},
//  /* 59 */ {  576460752303423433UL,    0x07FFFFFFFFFFFFFFUL},
//  /* 60 */ { 1152921504606846883UL,    0x0FFFFFFFFFFFFFFFUL},
//  /* 61 */ { 2305843009213693951UL,    0x1FFFFFFFFFFFFFFFUL},
//  /* 62 */ { 4611686018427387847UL,    0x3FFFFFFFFFFFFFFFUL},
//  /* 63 */ { 9223372036854775783UL,    0x7FFFFFFFFFFFFFFFUL},
//  /* 64 */ {18446744073709551557UL,    0xFFFFFFFFFFFFFFFFUL}
};

void swHashShiftSet (uint32_t shift, size_t *size, uint32_t *mod, uint32_t *mask)
{
  if (size && mod && mask)
  {
    *size   = 1 << shift;
    *mod    = primeModMask[shift].primeMod;
    *mask   = primeModMask[shift].mask;
  }
}

uint32_t swHashClosestShiftFind (uint32_t n)
{
  uint32_t i = 0;
  for (; n; i++)
    n >>= 1;
  return i;
}

uint32_t swHashPointerHash(const void *data)
{
  return swDJBAlgoHash(&data, sizeof(data));
}
