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

#define swStaticStringDefineEempty                    {.len = 0, .data = NULL}
#define swStaticStringDefine(str)                     {.len = sizeof(str) - 1, .data = str}
#define swStaticStringDefineFromCstr(str)             {.len = strlen(str), .data = str}
#define swStaticStringDefineWithLength(str, length)   {.len = length, .data = str}
#define swStaticStringSetWithLength(str, length)      *(swStaticString[]){{.len = length, .data = str}}
#define swStaticStringSetFromCstr(str)                *(swStaticString[]){{.len = strlen(str), .data = str}}
#define swStaticStringSet(str)                        *(swStaticStrig[]){{.len = sizeof(str), .data = str}}
#define swStaticStringSetEmpty                        *(swStaticSting[]){{.len = 0, .data = NULL}}

uint32_t swStaticStringHash(const swStaticString *string);
int swStaticStringCompare(const swStaticString *s1, const swStaticString *s2);
bool swStaticStringEqual(const swStaticString *s1, const swStaticString *s2);

#endif // SW_STORAGE_STATICSTRING_H