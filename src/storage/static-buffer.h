#ifndef SW_STORAGE_STATICBUFFER_H
#define SW_STORAGE_STATICBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct swStaticBuffer
{
  size_t len;
  char *data;
} swStaticBuffer;

#define swStaticBufferDefineEempty                    {.len = 0, .data = NULL}
#define swStaticBufferDefine(buf)                     {.len = sizeof(buf), .data = buf}
#define swStaticBufferDefineWithLength(str, length)   {.len = length, .data = str}
#define swStaticBufferSetWithLength(buf, length)      *(swStaticBuffer[]){{.len = length, .data = buf}}
#define swStaticBufferSet(buf)                        *(swStaticBuffer]){{.len = sizeof(buf), .data = buf}}
#define swStaticBufferSetEmpty                        *(swStaticBuffer]){{.len = 0, .data = NULL}}

uint32_t swStaticBufferHash(const swStaticBuffer *buffer);
int swStaticBufferCompare(const swStaticBuffer *b1, const swStaticBuffer *b2);
bool swStaticBufferEqual(const swStaticBuffer *b1, const swStaticBuffer *b2);

#endif // SW_STORAGE_STATICBUFFER_H
