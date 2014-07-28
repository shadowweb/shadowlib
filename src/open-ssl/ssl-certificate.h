#ifndef SW_OPENSSL_SSLCERTIFICATE_H
#define SW_OPENSSL_SSLCERTIFICATE_H

#include "openssl/x509.h"

#include "storage/static-string.h"
#include "storage/static-buffer.h"

typedef X509  swSSLCertificate;

static inline swSSLCertificate *swSSLCertificateNew() { return X509_new(); }
static inline void swSSLCertificateDelete(swSSLCertificate *certificate)
{
  if (certificate)
    X509_free(certificate);
}

swSSLCertificate *swSSLCertificateNewFromPEM(swStaticString *string);
swSSLCertificate *swSSLCertificateNewFromDER(swStaticBuffer *buffer);

typedef X509_STORE_CTX swSSLCertificateStoreContext;

typedef int (*swSSLCertificateVerifyCallback)(int result, swSSLCertificateStoreContext *context);

#endif // SW_OPENSSL_SSLCERTIFICATE_H