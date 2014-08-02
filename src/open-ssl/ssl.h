#ifndef SW_OPENSSL_SSL_H
#define SW_OPENSSL_SSL_H

#include <stdbool.h>

#include <openssl/ssl.h>

#include "io/socket.h"
#include "open-ssl/ssl-context.h"

typedef SSL swSSL;

static inline swSSL *swSSLNew(swSSLContext *context) { return SSL_new(context); }
static inline void swSSLDelete(swSSL *ssl) { SSL_free(ssl); }
static inline void swSSLSetAccept(swSSL *ssl) { SSL_set_accept_state(ssl); }
static inline void swSSLSetConnect(swSSL *ssl) { SSL_set_connect_state(ssl); }
static inline bool swSSLSetFD(swSSL *ssl, int fd) { return SSL_set_fd(ssl, fd); }
static inline void swSSLHandshake(swSSL *ssl) { SSL_do_handshake(ssl); }
static inline bool swSSLShutdown(swSSL *ssl) { return SSL_shutdown(ssl); }

swSSL *swSSLNewFromFD(swSSLContext *context, int fd, bool client);
swSocketReturnType swSSLRead  (swSSL *ssl, swStaticBuffer *buffer, ssize_t *bytesRead);
swSocketReturnType swSSLWrite (swSSL *ssl, swStaticBuffer *buffer, ssize_t *bytesWritten);

#endif // SW_OPENSSL_SSL_H