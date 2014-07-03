#ifndef SW_OPENSSL_SSLCONTEXT_H
#define SW_OPENSSL_SSLCONTEXT_H

#include <stdbool.h>

#include "openssl/ssl.h"

typedef SSL_CTX swSSLContext;

typedef enum swSSLMethod
{
  swSSLMethodNone,
  swSSLMethodSSLV2,
  swSSLMethodSSLV3,
  swSSLMethodSSLV23,
  swSSLMethodTLSV1,
  swSSLMethodTLSV11,
  swSSLMethodTLSV12,
  swSSLMethodDTLSV1,
  swSSLMethodMax
} swSSLMethod;

typedef enum swSSLMethodMode
{
  swSSLMethodModeNone,
  swSSLMethodModeClient,
  swSSLMethodModeServer,
  swSSLMethodModeMax
} swSSLMethodMode;

swSSLContext *swSSLContextNew(swSSLMethod method, swSSLMethodMode mode);
static inline swSSLContext *swSSLContextNewDefault() { return swSSLContextNew(swSSLMethodSSLV23, swSSLMethodModeNone); }
static inline void swSSLContextDelete(swSSLContext *context) { SSL_CTX_free(context); }

// TODO: add x509 and private/public key classes
// TODO: add setting up CA certificate and key
// TODO: set up certificate used for handshake (client or server)

#endif // SW_OPENSSL_SSLCONTEXT_H
