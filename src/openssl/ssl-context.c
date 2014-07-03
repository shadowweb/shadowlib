#include "ssl-context.h"

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
