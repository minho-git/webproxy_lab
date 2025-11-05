#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <pthread.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_item{
    char *uri; // 실제 공간은 malloc으로 할당해주기.
    char *data; // 실제 공간은 malloc으로 할당해주기.
    struct cache_item *next;
    struct cache_item *prev;
    size_t size;
} CacheItem;

typedef struct {
    CacheItem *head;
    CacheItem *rear;
    size_t total_size;
    pthread_rwlock_t rwlock;
} Cache;


/* Function Prototypes */
void cache_init(Cache *cache);
int cache_find (char *uri, char *response_buf, size_t *response_size, Cache *cache);
void cache_delete(Cache *cache);
void cache_store (Cache *cache , char *uri, char*data, size_t size);

#endif /* CACHE_H */
