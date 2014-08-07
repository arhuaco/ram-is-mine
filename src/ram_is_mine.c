#define _GNU_SOURCE

/*
 *  Nelson Castillo <nelsoneci@gmail.com>
 *  http://arhuaco.org
 *  2014.
 *  This code is released under the license GPL v2.
 *
 * We use glibc __libc_malloc and friends during initialization.
 * We could use those instead of real_malloc and friends all the time,
 * but let's use dlsym in case somebody else (a wrapper) redefines
 * the functions.
 */

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

static void * (*real_malloc)(size_t);
static void   (*real_free)(void *ptr);
static void * (*real_calloc)(size_t nmemb, size_t size);
static void * (*real_realloc)(void *ptr, size_t size);

#define uthash_malloc(sz) real_malloc(sz)
#define uthash_free(ptr,sz) real_free(ptr)
#include <uthash.h>

enum {
  LOG_FATAL,
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_TRACE,
} curent_log_level = LOG_TRACE;

#define STR_TMP(x) #x
#define STR(x) STR_TMP(x)
#define LOG(level, format, ...) do {                                  \
  if (level <= curent_log_level) {                                    \
    fprintf(stderr, __FILE__ ":" STR(__LINE__) ": "                   \
    format "\n", ##__VA_ARGS__);                                      \
  }                                                                   \
} while(0)

#define DIE(format, ...) do {                                         \
  fprintf(stderr, __FILE__ ":" STR(__LINE__) ":"                      \
  format "\n", ##__VA_ARGS__);                                        \
  exit(0);                                                            \
} while(0)

static pthread_mutex_t initialization_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_spinlock_t spin_lock_internal;

#define spin_lock() while (pthread_spin_lock(&spin_lock_internal))
#define spin_unlock() while (pthread_spin_unlock(&spin_lock_internal))

typedef struct {
  /* The pointer address is the key for the hash. */
  const void *ptr;
  /* Allocated size. */
  size_t size;
  /* Internal of uthash. */
  UT_hash_handle hh;
} MemInfo;

/* The hash of known allocated addresses. */
static MemInfo *mem_info_hash;

/* How much memory has been allocated. */
static volatile size_t memory_count;

/* Default memory Limit of 2GiB. Override with env MY_RAM_LIMIT. */
static size_t memory_limit = 1024L * 1024L * 1024L * 2L;

static void info_add(const void *ptr, size_t size) {
  MemInfo *info = real_malloc(sizeof(MemInfo));
  assert(info);
  info->ptr = ptr;
  info->size = size;
  HASH_ADD_PTR(mem_info_hash, ptr, info);
}

static MemInfo *info_find(const void *ptr) {
  MemInfo *info = NULL;
  HASH_FIND_PTR(mem_info_hash, &ptr, info);
  if (!info) {
    LOG(LOG_ERROR, "Pointer %p not found", ptr);
  }
  return info;
}

static void info_delete(MemInfo *info) {
  HASH_DEL(mem_info_hash, info);
}

static bool is_initializing;

static void init(void) {
  char *ram_limit;

  if (real_malloc) {
    return;
  }
  pthread_mutex_lock(&initialization_mutex);
  /* I think this double check is an anti-pattern.
   * Let's do it here. Nobody is watching. And it will save time.
   */
  if (real_malloc) {
    pthread_mutex_unlock(&initialization_mutex);
    return;
  }

  is_initializing = true;

  LOG(LOG_INFO, "** Initializing **");

  LOG(LOG_INFO, "USING uthash version " STR(UTHASH_VERSION));

  ram_limit = getenv ("MY_RAM_LIMIT");

  if (ram_limit) {
    memory_limit = atoll(ram_limit); /* TODO: Use strtoll and validate. */
    LOG(LOG_INFO, "MY_RAM_LIMIT set. Using: %zu", memory_limit);
  } else {
    LOG(LOG_INFO, "MY_RAM_LIMIT ***NOT*** set. Using default of: %zu. "
                  "Override with env MY_RAM_LIMIT.", memory_limit);
  }


  /* Initialize the spinlock first. */
  if(pthread_spin_init(&spin_lock_internal, PTHREAD_PROCESS_SHARED)) {
    DIE("Error in `pthread_spin_init`");
  }

  if (!((real_malloc = dlsym(RTLD_NEXT, "malloc")) &&
        (real_free = dlsym(RTLD_NEXT, "free")) &&
        (real_calloc = dlsym(RTLD_NEXT, "calloc")) &&
        (real_realloc = dlsym(RTLD_NEXT, "realloc"))
       )) {
    DIE("Error in `dlsym`: %s", dlerror());
  }

  is_initializing = false;

  pthread_mutex_unlock(&initialization_mutex);
}


void *malloc_and_calloc(size_t size, bool is_calloc) {
  init();

  spin_lock();
  if (memory_count + size > memory_limit) {
    spin_unlock();
    /* Idential LOG in realloc. Factorize? */
    LOG(LOG_INFO, "Allocation denied! We have allocated %zu bytes, limit is %zu and requested %zu more.",
        memory_count, memory_limit, size);
    return NULL;
  }
  spin_unlock();

  void *p = is_calloc ? real_calloc(1, size) : real_malloc(size);
  if (p) {
    spin_lock();
    memory_count += size;
    spin_unlock();
    LOG(LOG_TRACE, "Allocated %zu bytes. Currenty allocated: %zu.", size, memory_count);
    info_add(p, size);
  } else {
    LOG(LOG_FATAL, "Allocation failed!. We have allocated %zu bytes, requested %zu more.", memory_count, size);
  }
  return p;
}

void *malloc(size_t size) {
  if (is_initializing) {
    extern void *__libc__malloc(size_t);
    return __libc__malloc(size);
  }
  return malloc_and_calloc(size, false /* is alloc? */);
}

void *calloc(size_t nmemb, size_t size) {
  if (is_initializing) {
    extern void *__libc_calloc(size_t, size_t);
    return __libc_calloc(nmemb, size);
  }
  return malloc_and_calloc(size * nmemb, true /* is alloc? */);
}

void free(void *ptr) {
  if (is_initializing) {
    extern void __libc_free(void *);
    __libc_free(ptr);
    return;
  }
  if (ptr == NULL)
    return;

  init();

  MemInfo *info = info_find(ptr);
  if(info) {
    real_free(ptr);
    spin_lock();
    memory_count -= info->size;
    spin_unlock();
    LOG(LOG_TRACE, "Free'd %zu bytes. Using %zu.", info->size, memory_count);
    info_delete(info);
  } else {
    LOG(LOG_ERROR, "Tried to free unknown pointer %p.", ptr);
  }
}

void *realloc(void *old_ptr, size_t new_size) {
  if (is_initializing) {
    extern void *__libc_realloc(void *, size_t);
    return __libc_realloc(old_ptr, new_size);
  }

  if (old_ptr == NULL)
    return malloc(new_size);

  if (new_size == 0) {
    free(old_ptr);
    return NULL;
  }


  MemInfo *old_info = info_find(old_ptr);
  if (old_info == NULL) {
    LOG(LOG_ERROR, "Tried to realloc unknown pointer %p.", old_ptr);
    return NULL;
  }

  if (memory_count - old_info->size + new_size > memory_limit) {
    /* Idential LOG in malloc_or_calloc. Factorize? */
    LOG(LOG_INFO, "Allocation denied! We have allocated %zu bytes, limit is %zu and requested %zu more.",
        memory_count, memory_limit,  old_info->size + new_size);
   return NULL; /* old_info untouched. This is OK. */
  }

  void *new_ptr = real_realloc(old_ptr, new_size);

  if (new_ptr == NULL) {
    LOG(LOG_ERROR, "Realloc of %p failed. Memory (should be) untouched.", old_ptr);
    return NULL;
  }

  spin_lock();
  memory_count -= old_info->size;
  memory_count += new_size;
  spin_unlock();

  const size_t old_size = old_info->size;

  if (new_ptr == old_ptr) {
    old_info->size = new_size;
  } else {
    info_delete(old_info);
    info_add(new_ptr, new_size);
  }

  LOG(LOG_TRACE, "Reallocated from %zu bytes to %zu bytes. Currently allocated %zu bytes. Old ptr %p, new ptr %p.",
                  old_size, new_size, memory_count, old_ptr, new_ptr);

  return new_ptr;
}
