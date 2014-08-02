#ifndef SW_OPENSSL_SSLCONTEXT_H
#define SW_OPENSSL_SSLCONTEXT_H

#include <stdbool.h>

#include <openssl/ssl.h>

#include "open-ssl/ssl-certificate.h"
#include "open-ssl/ssl-key.h"

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

typedef enum swSSLVerifyMode
{
  swSSLVerifyModeNone           = SSL_VERIFY_NONE,
  swSSLVerifyModePeer           = SSL_VERIFY_PEER,
  swSSLVerifyModeNoPeerCertFail = SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
  swSSLVerifyModeClientOnce     = SSL_VERIFY_CLIENT_ONCE
} swSSLVerifyMode;


swSSLContext *swSSLContextNew(swSSLMethod method, swSSLMethodMode mode);
static inline swSSLContext *swSSLContextNewDefault() { return swSSLContextNew(swSSLMethodSSLV23, swSSLMethodModeNone); }
static inline void swSSLContextDelete(swSSLContext *context) { SSL_CTX_free(context); }

static inline void swSSLContextSetOptions(swSSLContext *context, long options) { if (context) SSL_CTX_set_options(context, options); }
static inline long swSSLContextGetOptions(swSSLContext *context )
{
  if (context)
    return SSL_CTX_get_options(context);
  return 0;
}
static inline void swSSLContextClearOptions(swSSLContext *context, long options) { if (context) SSL_CTX_clear_options(context, options); }

static inline bool swSSLContextSetCiphers(swSSLContext *context, const char *cipherList) { return SSL_CTX_set_cipher_list(context, cipherList); }
static inline void swSSLContextSetVerify(swSSLContext *context, uint32_t verifyMode, swSSLCertificateVerifyCallback verifyCallback)
{
  if (context)
    SSL_CTX_set_verify(context, verifyMode, (int (*)(int, X509_STORE_CTX *))verifyCallback);
}

static inline bool swSSLContextSetCertificate(swSSLContext *context, swSSLCertificate *cert)
{
  if (context && cert)
    return SSL_CTX_use_certificate(context ,cert);
  return false;
}
static inline bool swSSLContextSetKey(swSSLContext *context, swSSLKey *key)
{
  if (context && key)
    return SSL_CTX_use_PrivateKey(context, key);
  return false;
}

bool swSSLContextLoadVerifyCertificates(swSSLContext *context, swStaticString *certificatesString);

#endif // SW_OPENSSL_SSLCONTEXT_H
