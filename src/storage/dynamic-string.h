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

bool swDynamicStringEnsureSize(swDynamicString *string, size_t size);

swDynamicString *swDynamicStringNewFromStaticString(const swStaticString *staticStr);
bool swDynamicStringSetFromStaticString(swDynamicString *dynamicStr, const swStaticString *staticStr);
swDynamicString *swDynamicStringNewFromCString(const char *cStr);
bool swDynamicStringSetFromCString(swDynamicString *dynamicStr, const char *cStr);
swDynamicString *swDynamicStringNewFromFormat(const char *format, ...) __attribute__ ((format(printf, 1, 2)));
bool swDynamicStringSetFromFormat(swDynamicString *dynamicStr, const char *format, ...)  __attribute__ ((format(printf, 2, 3)));

void swDynamicStringClear(swDynamicString *string);
void swDynamicStringRelease(swDynamicString *string);

bool swDynamicStringAppendStaticString(swDynamicString *dynamicStr, const swStaticString *staticStr);
bool swDynamicStringAppendCString(swDynamicString *dynamicStr, const char *cStr);

#define SW_TIME_STRING_SIZE  23

bool swDynamicStringAppendTime(swDynamicString *dynamicStr);

// TODO: do the same for buffer with minor changes
// TODO: implement to and from hex, to and from base64
// TODO: implement init fuction

#endif // SW_STORAGE_DYNAMICSTRING_H
