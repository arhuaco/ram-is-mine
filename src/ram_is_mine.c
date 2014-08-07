#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

static void * (*real_malloc)(size_t);

#define uthash_malloc(sz) real_malloc(sz)
#include <uthash.h>


enum {
  LOG_FATAL,
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_TRACE,
} curent_log_level = LOG_TRACE;

#define STR2(x) #x
#define STR(x) STR2(x)
#define LOG(level, format, ...) do {                                  \
  if (level <= curent_log_level) {                                    \
    fprintf(stderr, __FILE__ ":" STR(__LINE__) ": "                   \
    format "\n", ##__VA_ARGS__);                                      \
  }                                                                   \
} while(0)

#define DIE(format, ...) do {                                         \
  fprintf(stderr, __FILE__ ":" STR(__LINE__) ":"                      \
  format "\n", ##__VA_ARGS__);                                        \
  exit(1);                                                            \
} while(0)

static pthread_mutex_t initialization_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_spinlock_t spin_lock;

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
volatile size_t memory_count;

bool info_find(const void *ptr, size_t *size) {
  MemInfo *info = NULL;
  HASH_FIND_PTR(mem_info_hash, &ptr, info);
  if (info) {
    *size = info->size;
    return true;
  }
  LOG(LOG_ERROR, "Pointer %p not found", ptr);
  return false;
}


static void info_add(const void *ptr, size_t size) {
  MemInfo *info = real_malloc(sizeof(MemInfo));
  assert(info);
  info->ptr = ptr;
  info->size = size;
  HASH_ADD_PTR(mem_info_hash, ptr, info);
  LOG(LOG_TRACE, "Added pointer %p", ptr);
}

static void intersect_init(void) {
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

  /* Initialize the spinlock first. */
  if(pthread_spin_init(&spin_lock, PTHREAD_PROCESS_SHARED)) {
    fprintf(stderr, "Error in `pthread_spin_init`\n");
    exit(1);
  }

  real_malloc = dlsym(RTLD_NEXT, "malloc");
  if (NULL == real_malloc) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    exit(1);
  }

  pthread_mutex_unlock(&initialization_mutex);
}

void *malloc(size_t size) {

  intersect_init();

  fprintf(stderr, "malloc(%zu) = ", size);

  while (pthread_spin_lock(&spin_lock));
  if (memory_count > 2L * 1024L * 1024L * 1024L) {
    while (pthread_spin_unlock(&spin_lock));
    fprintf(stderr, " DENY!\n");
    return NULL;
  }
  while (pthread_spin_unlock(&spin_lock));


  void *p = real_malloc(size);
  if (p) {
    fprintf(stderr, " OK\n");
    while (pthread_spin_lock(&spin_lock));
    memory_count += size;
    while (pthread_spin_unlock(&spin_lock));
  } else {
    fprintf(stderr, " FAIL\n");
  }

  fprintf(stderr, "allocated: %zu\n", memory_count);

  info_add(p, size);
  return p;
}

/*void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);*/
