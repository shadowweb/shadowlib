#include "ssl-certificate.h"

#include <openssl/bio.h>
#include <openssl/pem.h>

swSSLCertificate *swSSLCertificateNewFromPEM(swStaticString *string)
{
  swSSLCertificate *cert = NULL;
  if (string)
  {
    BIO *pemBIO = BIO_new_mem_buf(string->data, string->len);
    if (pemBIO)
    {
      cert = PEM_read_bio_X509(pemBIO, NULL, NULL, NULL);
      BIO_free_all(pemBIO);
    }
  }
  return cert;
}

swSSLCertificate *swSSLCertificateNewFromDER(swStaticBuffer *buffer)
{
  swSSLCertificate *cert = NULL;
  if (buffer)
  {
    BIO *pemBIO = BIO_new_mem_buf(buffer->data, buffer->len);
    if (pemBIO)
    {
      cert = d2i_X509_bio(pemBIO, NULL);
      BIO_free_all(pemBIO);
    }
  }
  return cert;
}
