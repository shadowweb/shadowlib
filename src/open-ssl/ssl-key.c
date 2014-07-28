#include "ssl-key.h"

#include <openssl/bio.h>
#include <openssl/pem.h>

swSSLKey *swSSLKeyNewFromPEM(swStaticString *string)
{
  swSSLKey *key = NULL;
  if (string)
  {
    BIO *pemBIO = BIO_new_mem_buf(string->data, string->len);
    if (pemBIO)
    {
      key = PEM_read_bio_PrivateKey(pemBIO, NULL, NULL, NULL);
      BIO_free_all(pemBIO);
    }
  }
  return key;
}

swSSLKey *swSSLKeyNewFromDER(swStaticBuffer *buffer)
{
  swSSLKey *key = NULL;
  if (buffer)
  {
    BIO *pemBIO = BIO_new_mem_buf(buffer->data, buffer->len);
    if (pemBIO)
    {
      key = d2i_PrivateKey_bio(pemBIO, NULL);
      BIO_free_all(pemBIO);
    }
  }
  return key;
}
