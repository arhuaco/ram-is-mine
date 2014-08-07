#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

static void * (*real_malloc)(size_t);
static void   (*real_free)(void *ptr);
static void * (*real_calloc)(size_t nmemb, size_t size);
/*void *realloc(void *ptr, size_t size);*/

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

#define STR(x) #x
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

static bool info_find(const void *ptr, size_t *size) {
  MemInfo *info = NULL;
  HASH_FIND_PTR(mem_info_hash, &ptr, info);
  if (info) {
    *size = info->size;
    return true;
  }
  LOG(LOG_ERROR, "Pointer %p not found", ptr);
  return false;
}

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

  real_malloc = dlsym(RTLD_NEXT, "malloc");
  real_free = dlsym(RTLD_NEXT, "free");
  real_calloc = dlsym(RTLD_NEXT, "calloc");

  if (real_malloc == NULL || real_free == NULL || real_calloc == NULL) {
    DIE("Error in `dlsym`: %s", dlerror());
  }

  pthread_mutex_unlock(&initialization_mutex);
}

void *malloc(size_t size) {
  init();

  spin_lock();
  if (memory_count + size > memory_limit) {
    spin_unlock();
    LOG(LOG_INFO, "Allocation denied! We have allocated %zu bytes, limit is %zu and requested %zu more.",
        memory_count, memory_limit, size);
    return NULL;
  }
  spin_unlock();

  void *p = real_malloc(size);
  if (p) {
    spin_lock();
    memory_count += size;
    spin_unlock();
    LOG(LOG_TRACE, "Allocated %zu bytes", size);
    info_add(p, size);
  } else {
    LOG(LOG_FATAL, "Real alloc failed!. We have allocated %zu bytes, requested %zu more", memory_count, size);
  }
  return p;
}

void free(void *ptr) {
  if (ptr == NULL)
    return;

  size_t size;
  if(info_find(ptr, &size)) {
    spin_lock();
    memory_count -= size;
    spin_unlock();
    LOG(LOG_TRACE, "Free'd %zu bytes. Using %zu.", size, memory_count);
  } else {
    LOG(LOG_ERROR, "Tried to free unknown pointer %p", ptr);
  }
}


void *calloc(size_t nmemb, size_t size) {
  return malloc(nmemb * size);
}
/*void *realloc(void *ptr, size_t size);*/
