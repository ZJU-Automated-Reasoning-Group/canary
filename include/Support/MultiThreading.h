#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include <pthread.h>

typedef pthread_t PPThreadIDType;

class PPMutex {
private:
  pthread_mutex_t mutex;
  // "is_valid = true" means that the object is ready for using
  bool is_valid;

public:
  PPMutex();
  ~PPMutex();

  // return true if the object is ready for using
  bool isValid();

  // lock/unlock, return true if succeeded
  bool lock();
  bool unlock();
};

class PPReadWriteLock {
private:
  pthread_rwlock_t mutex;
  // is_valid = true means that the object is ready for using
  bool is_valid;

public:
  PPReadWriteLock();
  ~PPReadWriteLock();

  // return true if the object is ready for using
  bool isValid();

  // read/write lock and unlock, return true if succeeded
  bool rdLock();
  bool wrLock();
  bool rdUnlock();
  bool wrUnlock();
};

class PPThreadData {
private:
  pthread_key_t key;
  // is_valid = true means that the object is ready for using
  bool is_valid;

public:
  PPThreadData(void (*destructor)(void *) = nullptr);
  ~PPThreadData();

  // return true if the object is ready for using
  bool isValid();

  // Set the thread specific data as a void*
  // return true if succeeded
  bool setValue(void *data);

  // Get the thread specific data as a void*
  // Note: This function shall return null if the value dose not exist or error
  // happens
  void *getValue();
};

// return the executing thread ID of the caller
PPThreadIDType getCurrentThreadID();

#endif // MULTITHREADING_H