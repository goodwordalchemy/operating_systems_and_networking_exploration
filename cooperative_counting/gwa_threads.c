#include <assert.h>
#include <pthread.h>

#include "gwa_threads.h"

void wrapped_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void*), void *arg)
{
    int rc = pthread_create(thread, attr, start_routine, arg);
    assert(rc == 0);
}

void wrapped_pthread_join(pthread_t thread, void **value_ptr)
{
    int rc = pthread_join(thread, value_ptr);
    assert(rc == 0);
}

void wrapped_pthread_mutex_lock(pthread_mutex_t *m){
    int rc = pthread_mutex_lock(m);
    assert(rc == 0);
}

void wrapped_pthread_mutex_unlock(pthread_mutex_t *m){
    int rc = pthread_mutex_unlock(m);
    assert(rc == 0);
}

void wrapped_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex){
    int rc = pthread_cond_wait(cond, mutex);
    assert(rc == 0);
}

void wrapped_pthread_cond_signal(pthread_cond_t *cond){
    int rc = pthread_cond_signal(cond);
    assert(rc == 0);
}
