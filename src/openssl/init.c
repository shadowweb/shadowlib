#include "init.h"

#include "core/memory.h"

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/conf.h>

#include <pthread.h>

static pthread_rwlock_t *swLocks = NULL;

static void swOpenSSLLockingCallback(int mode, int type, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
  {
    if (mode & CRYPTO_READ)
      pthread_rwlock_rdlock(&swLocks[type]);
    else
      pthread_rwlock_wrlock(&swLocks[type]);
  }
  else
    pthread_rwlock_unlock(&swLocks[type]);
}

struct CRYPTO_dynlock_value
{
  pthread_rwlock_t lock;
};

static struct CRYPTO_dynlock_value *swOpenSSLDynamicLockCreateCallback(const char *file, int line)
{
  struct CRYPTO_dynlock_value *value = swMemoryCalloc(1, sizeof(struct CRYPTO_dynlock_value));
  if (value)
    pthread_rwlock_init(&value->lock, NULL);
  return value;
}

static void swOpenSSLDynamicLockDestroyCallback(struct CRYPTO_dynlock_value *value, const char *file, int line)
{
  if (value)
  {
    pthread_rwlock_destroy(&value->lock);
    swMemoryFree(value);
  }
}

static void swOpenSSLDynamicLockLockCallback(int mode, struct CRYPTO_dynlock_value *value, const char *file, int line)
{
  if (value)
  {
    if (mode & CRYPTO_LOCK)
    {
      if (mode & CRYPTO_READ)
        pthread_rwlock_rdlock(&value->lock);
      else
        pthread_rwlock_wrlock(&value->lock);
    }
    else
      pthread_rwlock_unlock(&value->lock);
  }
}

bool swOpenSSLStart()
{
  bool rtn = false;
  SSL_library_init();
  SSL_load_error_strings();
  OPENSSL_add_all_algorithms_conf();
  CRYPTO_set_id_callback((unsigned long(*)())pthread_self);
  RAND_poll();
  if ((swLocks = swMemoryCalloc(CRYPTO_num_locks(), sizeof(pthread_rwlock_t))))
  {
    for (int i = 0; i < CRYPTO_num_locks(); i++)
      pthread_rwlock_init(&swLocks[i], NULL);
    CRYPTO_set_locking_callback(swOpenSSLLockingCallback);
    CRYPTO_set_dynlock_create_callback(swOpenSSLDynamicLockCreateCallback);
    CRYPTO_set_dynlock_destroy_callback(swOpenSSLDynamicLockDestroyCallback);
    CRYPTO_set_dynlock_lock_callback(swOpenSSLDynamicLockLockCallback);
    rtn = true;
  }
  return rtn;
}

void swOpenSSLStop()
{
  ERR_remove_thread_state(NULL);
  CONF_modules_unload(1);
  ERR_free_strings();
  EVP_cleanup();
  OBJ_cleanup();
  CRYPTO_cleanup_all_ex_data();

  CRYPTO_set_dynlock_create_callback(NULL);
  CRYPTO_set_dynlock_lock_callback(NULL);
  CRYPTO_set_dynlock_destroy_callback(NULL);

  CRYPTO_set_locking_callback(NULL);
  CRYPTO_set_id_callback(NULL);
  for (int i = 0; i < CRYPTO_num_locks(); i++)
      pthread_rwlock_destroy(&swLocks[i]);
  swMemoryFree(swLocks);
}
