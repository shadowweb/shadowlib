#include "open-ssl/ssl.h"

#include <openssl/err.h>

swSSL *swSSLNewFromFD(swSSLContext *context, int fd, bool client)
{
  swSSL *rtn = NULL;
  if (context && fd >= 0)
  {
    swSSL *newSSL = swSSLNew(context);
    if (newSSL)
    {
      if (swSSLSetFD(newSSL, fd))
      {
        if (client)
          swSSLSetConnect(newSSL);
        else
          swSSLSetAccept(newSSL);
        rtn = newSSL;
      }
      else
        swSSLDelete(newSSL);
    }
  }
  return rtn;

}

static swSocketReturnType swSSLGetError(swSSL *ssl, int ret)
{
  swSocketReturnType rtn = swSocketReturnNone;
  int errorCode = SSL_get_error(ssl, ret);
  switch (errorCode)
  {
    case SSL_ERROR_WANT_READ:
      rtn = swSocketReturnReadNotReady;
      break;
    case SSL_ERROR_WANT_WRITE:
      rtn = swSocketReturnWriteNotReady;
      break;
    case SSL_ERROR_WANT_CONNECT:
    case SSL_ERROR_WANT_ACCEPT:
    case SSL_ERROR_WANT_X509_LOOKUP:
      rtn = swSocketReturnNotReady;
      break;
    case SSL_ERROR_ZERO_RETURN:
      rtn = swSocketReturnClose;
      break;
    case SSL_ERROR_SYSCALL:
      if (!ERR_peek_error())
      {
        rtn = swSocketReturnClose;
        break;
      }
    default:
      rtn = swSocketReturnError;
  }
  return rtn;
}

// When an SSL_read() operation has to be repeated because of SSL_ERROR_WANT_READ or
// SSL_ERROR_WANT_WRITE, it must be repeated with the same arguments.
swSocketReturnType swSSLRead(swSSL *ssl, swStaticBuffer *buffer, ssize_t *bytesRead)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (ssl && buffer)
  {
    int ret = SSL_read(ssl, buffer->data, buffer->len);
    if (ret > 0)
    {
      rtn = swSocketReturnOK;
      if (bytesRead)
      {
        if ((size_t)ret > buffer->len)
          *bytesRead = buffer->len;
        else
          *bytesRead = ret;
      }
    }
    else
      rtn = swSSLGetError(ssl, ret);
  }
  return rtn;
}

// When an SSL_write() operation has to be repeated because of SSL_ERROR_WANT_READ or
// SSL_ERROR_WANT_WRITE, it must be repeated with the same arguments.
swSocketReturnType swSSLWrite(swSSL *ssl, swStaticBuffer *buffer, ssize_t *bytesWritten)
{
  swSocketReturnType rtn = swSocketReturnNone;
  if (ssl && buffer)
  {
    int ret = SSL_write(ssl, buffer->data, buffer->len);
    if (ret > 0)
    {
      rtn = swSocketReturnOK;
      if (bytesWritten)
        *bytesWritten = ret;
    }
    else
      rtn = swSSLGetError(ssl, ret);
  }
  return rtn;

}
