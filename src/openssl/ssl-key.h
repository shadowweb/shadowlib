#ifndef SW_OPENSSL_SSLKEY_H
#define SW_OPENSSL_SSLKEY_H

#include "openssl/x509.h"

#include "storage/static-string.h"
#include "storage/static-buffer.h"

typedef EVP_PKEY  swSSLKey;

static inline swSSLKey *swSSLKeyNew() { return EVP_PKEY_new(); }
static inline void swSSLKeyDelete(swSSLKey *key)
{
  if (key)
    EVP_PKEY_free(key);
}

swSSLKey *swSSLKeyNewFromPEM(swStaticString *string);
swSSLKey *swSSLKeyNewFromDER(swStaticBuffer *buffer);

#endif // SW_OPENSSL_SSLKEY_H