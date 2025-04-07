#include "Support/MultiThreading.h"

PPMutex::PPMutex() {
  int res = pthread_mutex_init(&mutex, nullptr);
  is_valid = (res == 0);
}

PPMutex::~PPMutex() {
  pthread_mutex_destroy(&mutex);
  is_valid = false;
}

bool PPMutex::isValid() { return is_valid; }

bool PPMutex::lock() {
  if (!isValid()) {
    return false;
  }

  int res = pthread_mutex_lock(&mutex);
  return res == 0;
}

bool PPMutex::unlock() {
  if (!isValid()) {
    return false;
  }

  int res = pthread_mutex_unlock(&mutex);
  return res == 0;
}

PPReadWriteLock::PPReadWriteLock() {
  int res = pthread_rwlock_init(&mutex, nullptr);
  is_valid = (res == 0);
}

PPReadWriteLock::~PPReadWriteLock() {
  pthread_rwlock_destroy(&mutex);
  is_valid = false;
}

bool PPReadWriteLock::isValid() { return is_valid; }

bool PPReadWriteLock::rdLock() {
  if (!isValid()) {
    return false;
  }

  int res = pthread_rwlock_rdlock(&mutex);
  return res == 0;
}

bool PPReadWriteLock::wrLock() {
  if (!isValid()) {
    return false;
  }

  int res = pthread_rwlock_wrlock(&mutex);
  return res == 0;
}

bool PPReadWriteLock::rdUnlock() {
  if (!isValid()) {
    return false;
  }

  int res = pthread_rwlock_unlock(&mutex);
  return res == 0;
}

bool PPReadWriteLock::wrUnlock() {
  if (!isValid()) {
    return false;
  }

  int res = pthread_rwlock_unlock(&mutex);
  return res == 0;
}

PPThreadData::PPThreadData(void (*destructor)(void *)) {
  int res = pthread_key_create(&key, destructor);
  is_valid = (res == 0);
}

PPThreadData::~PPThreadData() {
  pthread_key_delete(key);
  is_valid = false;
}

bool PPThreadData::isValid() { return is_valid; }

bool PPThreadData::setValue(void *data) {
  if (!isValid()) {
    return false;
  }

  int res = pthread_setspecific(key, data);
  return res == 0;
}

void *PPThreadData::getValue() {
  if (!isValid()) {
    return nullptr;
  }

  return pthread_getspecific(key);
}

PPThreadIDType getCurrentThreadID() { return pthread_self(); }