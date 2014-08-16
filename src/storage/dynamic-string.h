#ifndef SW_STORAGE_DYNAMICSTRING_H
#define SW_STORAGE_DYNAMICSTRING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "storage/static-string.h"

// TODO: create swFixedString that looks the same, but allocates the whole data structure with the
//        buffer in a single malloc

typedef struct swDynamicString
{
  size_t len;
  char *data;
  size_t size;
} swDynamicString;

static inline uint32_t swDynamicStringHash(const swDynamicString *string)
{
  return swStaticStringHash((swStaticString *)(string));
}

static inline int swDynamicStringCompare(const swDynamicString *s1, const swDynamicString *s2)
{
  return swStaticStringCompare((swStaticString *)(s1), (swStaticString *)(s2));
}

static inline bool swDynamicStringEqual(const swDynamicString *s1, const swDynamicString *s2)
{
  return swStaticStringEqual((swStaticString *)(s1), (swStaticString *)(s2));
}

static inline bool swDynamicStringSame(const swDynamicString *s1, const swDynamicString *s2)
{
  return swStaticStringSame((swStaticString *)(s1), (swStaticString *)(s2));
}

swDynamicString *swDynamicStringNew(size_t size);
void swDynamicStringDelete(swDynamicString *string);

swDynamicString *swDynamicStringNewFromStaticString(const swStaticString *staticStr);
bool swDynamicStringSetFromStaticString(swDynamicString *dynamicStr, const swStaticString *staticStr);
swDynamicString *swDynamicStringNewFromCString(const char *cStr);
bool swDynamicStringSetFromCString(swDynamicString *dynamicStr, const char *cStr);

void swDynamicStringClear(swDynamicString *string);

// TODO: do the same for buffer with minoor changes
// TODO: implement to and from hex, to and from base64

#endif // SW_STORAGE_DYNAMICSTRING_H
