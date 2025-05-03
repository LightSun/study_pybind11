
#include "h_atomic.h"
//#define USE_MSC_ATOMICS 1

#if defined(USE_C11_ATOMICS)
#include <stdatomic.h>
#endif

#if defined(USE_MSC_ATOMICS)
#include <intrin.h>
#include <assert.h>
#endif

#if !defined(USE_MSC_ATOMICS) && !defined(USE_GCC_ATOMICS) && defined(USE_PTHREAD_ATOMICS)
#include <pthread.h>
static pthread_mutex_t ptm = PTHREAD_MUTEX_INITIALIZER;
#endif

//CompareAndSwap
int h_atomic_cas(int volatile *a, int oldvalue, int newvalue)
{
#if defined(USE_C11_ATOMICS)
  return atomic_compare_exchange_strong(a,
                                        &oldvalue, newvalue);
#elif defined(USE_MSC_ATOMICS)
  assert(sizeof(int) == sizeof(long));
  return (_InterlockedCompareExchange((long*)a, (long)newvalue, (long)oldvalue) == (long)oldvalue);
#elif defined(USE_GCC_ATOMICS)
  return __sync_bool_compare_and_swap(a, oldvalue, newvalue);
#elif defined(USE_PTHREAD_ATOMICS)
  int ret = 0;
  pthread_mutex_lock(&ptm);
  if(*a == oldvalue) {
    *a = newvalue;
    ret = 1;
  }
  pthread_mutex_unlock(&ptm);
  return ret;
#else
#warning atomic_cas is not thread safe
  if(*a == oldvalue) {
    *a = newvalue;
    return 1;
  }
  else
    return 0;
#endif
}
int h_atomic_add(int volatile *a, int value){
#if defined(USE_C11_ATOMICS)
  return atomic_fetch_add(a, value);
#elif defined(USE_MSC_ATOMICS)
  assert(sizeof(int) == sizeof(long));
  return _InterlockedExchangeAdd((long*)a, value);
#elif defined(USE_GCC_ATOMICS)
  return __sync_fetch_and_add(a, value);
#else
  int oldvalue;
  do {
    oldvalue = *a;
  } while (!h_atomic_cas(a, oldvalue, (oldvalue + value)));
  return oldvalue;
#endif
}

int h_atomic_get(int volatile *a)
{
#if defined(USE_C11_ATOMICS)
  return atomic_load(a);
#else
  int value;
  do {
    value = *a;
  } while (!h_atomic_cas(a, value, value));
  return value;
#endif
}

void h_atomic_set(int volatile *a, int newvalue)
{
#if defined(USE_C11_ATOMICS)
  atomic_store(a, newvalue);
#elif defined(USE_MSC_ATOMICS)
  assert(sizeof(int) == sizeof(long));
  _InterlockedExchange((long*)a, newvalue);
#elif defined(USE_GCC_ATOMICS)
  __sync_lock_test_and_set(a, newvalue);
#else
  int oldvalue;
  do {
    oldvalue = *a;
  } while (!h_atomic_cas(a, oldvalue, newvalue));
#endif
}
