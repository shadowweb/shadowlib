#include "open-ssl/ssl-context.h"

#include <openssl/x509_vfy.h>
#include <openssl/err.h>

typedef const SSL_METHOD *(*swSSLMethodCallback)(void);

static swSSLMethodCallback methodCallbacks[swSSLMethodModeMax][swSSLMethodMax] =
{
  [swSSLMethodModeNone] = {
    [swSSLMethodSSLV2  ] = SSLv2_method,
    [swSSLMethodSSLV3  ] = SSLv3_method,
    [swSSLMethodSSLV23 ] = SSLv23_method,
    [swSSLMethodTLSV1  ] = TLSv1_method,
    [swSSLMethodTLSV11 ] = TLSv1_1_method,
    [swSSLMethodTLSV12 ] = TLSv1_2_method,
    [swSSLMethodDTLSV1 ] = DTLSv1_method,
  },
  [swSSLMethodModeClient] = {
    [swSSLMethodSSLV2  ] = SSLv2_client_method,
    [swSSLMethodSSLV3  ] = SSLv3_client_method,
    [swSSLMethodSSLV23 ] = SSLv23_client_method,
    [swSSLMethodTLSV1  ] = TLSv1_client_method,
    [swSSLMethodTLSV11 ] = TLSv1_1_client_method,
    [swSSLMethodTLSV12 ] = TLSv1_2_client_method,
    [swSSLMethodDTLSV1 ] = DTLSv1_client_method,
  },
  [swSSLMethodModeServer] = {
    [swSSLMethodSSLV2  ] = SSLv2_server_method,
    [swSSLMethodSSLV3  ] = SSLv3_server_method,
    [swSSLMethodSSLV23 ] = SSLv23_server_method,
    [swSSLMethodTLSV1  ] = TLSv1_server_method,
    [swSSLMethodTLSV11 ] = TLSv1_1_server_method,
    [swSSLMethodTLSV12 ] = TLSv1_2_server_method,
    [swSSLMethodDTLSV1 ] = DTLSv1_server_method,
  }
};

swSSLContext *swSSLContextNew(swSSLMethod method, swSSLMethodMode mode)
{
  swSSLContext *rtn = NULL;
  if (method > swSSLMethodNone && method < swSSLMethodMax && mode >= swSSLMethodModeNone && mode < swSSLMethodModeMax)
    rtn = SSL_CTX_new((methodCallbacks[mode][method])());
  return rtn;
}

bool swSSLContextLoadVerifyCertificates(swSSLContext *context, swStaticString *certificatesString)
{
  bool rtn = false;
  if (context && certificatesString)
  {
    X509_STORE * certificatesStore = context->cert_store;
    if (certificatesStore)
    {
      BIO *input = BIO_new_mem_buf(certificatesString->data, certificatesString->len);
      if (input)
      {
        STACK_OF(X509_INFO) *certificatesInfo = PEM_X509_INFO_read_bio(input, NULL, NULL, NULL);
        if (certificatesInfo)
        {
          uint32_t count = 0;
          for(int i = 0; i < sk_X509_INFO_num(certificatesInfo); i++)
          {
            X509_INFO *certInfo = sk_X509_INFO_value(certificatesInfo, i);
            if (certInfo)
            {
              if (certInfo->x509)
              {
                X509_STORE_add_cert(certificatesStore, certInfo->x509);
                count++;
              }
              if (certInfo->crl)
              {
                X509_STORE_add_crl(certificatesStore, certInfo->crl);
                count++;
              }
            }
          }
          rtn = (count != 0);
        }
        else
          X509err(X509_F_X509_LOAD_CERT_CRL_FILE,ERR_R_PEM_LIB);
        BIO_free(input);
      }
      else
        X509err(X509_F_X509_LOAD_CERT_CRL_FILE,ERR_R_SYS_LIB);
    }
  }
  return rtn;
}
