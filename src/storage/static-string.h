#ifndef SW_STORAGE_STATICSTRING_H
#define SW_STORAGE_STATICSTRING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct swStaticString
{
  size_t len;
  char *data;
} swStaticString;

#define swStaticStringDefineEmpty                     {.len = 0, .data = NULL}
#define swStaticStringDefine(str)                     {.len = sizeof(str) - 1, .data = str}
#define swStaticStringDefineFromCstr(str)             {.len = strlen(str), .data = str}
#define swStaticStringDefineWithLength(str, length)   {.len = length, .data = str}

#define swStaticStringSetWithLength(str, length)      *(swStaticString[]){{.len = length, .data = str}}
#define swStaticStringSetFromCstr(str)                *(swStaticString[]){{.len = strlen(str), .data = str}}
#define swStaticStringSet(str)                        *(swStaticString[]){{.len = sizeof(str) - 1, .data = str}}
#define swStaticStringSetEmpty                        *(swStaticString[]){{.len = 0, .data = NULL}}

#define swStaticStringCharEqual(str, i, c)            (((i) < (str).len)? (str).data[(i)] == c : false)

uint32_t swStaticStringHash(const swStaticString *string);
int swStaticStringCompare(const swStaticString *s1, const swStaticString *s2);
int swStaticStringCompareCaseless(const swStaticString *s1, const swStaticString *s2);
bool swStaticStringEqual(const swStaticString *s1, const swStaticString *s2);
bool swStaticStringSame(const swStaticString *s1, const swStaticString *s2);

bool swStaticStringFindChar(const swStaticString *string, char c, size_t *position);
bool swStaticStringSetSubstring(const swStaticString *string, swStaticString *subString, size_t start, size_t end);

// TODO: may be make bit flags from those flags; implement it
typedef enum swStaticStringSearchConstraints
{
  swStaticStringSearchAllowFirst    = 0x01,
  swStaticStringSearchAllowLast     = 0x02,
  swStaticStringSearchAllowAdjacent = 0x04
} swStaticStringSearchConstraints;
bool swStaticStringSplitChar(const swStaticString *string, char c, swStaticString *slices, uint32_t slicesCount, uint32_t *foundSlicesCount, uint32_t flags);
bool swStaticStringCountChar(const swStaticString *string, char c, uint32_t *foundCount);

#endif // SW_STORAGE_STATICSTRING_H
