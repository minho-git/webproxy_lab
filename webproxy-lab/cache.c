#include <stdlib.h>
#include <string.h>
#include "cache.h" // cache.h 포함

/* <stddef.h>와 <pthread.h>는 cache.h를 통해 포함됩니다 */


void cache_init(Cache *cache) {

    cache->head = NULL;
    cache->rear = NULL;
    cache->total_size = 0;
    pthread_rwlock_init(&cache->rwlock, NULL);

}

int cache_find (char *uri, char *response_buf, size_t *response_size, Cache *cache) {
    int found = 0;

    pthread_rwlock_rdlock(&cache->rwlock);

    CacheItem *cur = cache->head;
    while (cur!= NULL) {
        if (strcmp(cur->uri, uri) == 0) {
            memcpy(response_buf, cur->data, cur->size);
            *response_size = cur->size;
            found = 1;
            break;
        }

        cur =  cur->next;
    }

    pthread_rwlock_unlock(&cache->rwlock);
    return found;

    // 이 결과가 0이면 서버로 요청보내기
    // 이 결과가 1이면 그 response_buf의 데이터 클라이언트에게 보내기
}

void cache_delete(Cache *cache) {
    CacheItem *removeItem = cache->rear;

    if (cache->rear == NULL) { // 캐시가 비었을때
        return;

    } else if (cache->head == cache->rear){ // 항목이 1개일 때
        cache->head = NULL;
        cache->rear = NULL;

    } else { // 항목이 2개일 때
        cache->rear = cache->rear->prev;
        cache->rear->next = NULL;

    }

    // 캐시 사이즈 줄이기
    cache->total_size -= removeItem->size;

    free(removeItem->data);
    free(removeItem->uri);
    free(removeItem);
}

void cache_store (Cache *cache , char *uri, char*data, size_t size) {

    pthread_rwlock_wrlock(&cache->rwlock);

    // 캐시 공간이 가득 찼는지 확인
    while (cache->total_size + size > MAX_CACHE_SIZE) {
        cache_delete(cache);
    }

    // 새로운 CacheItem을 생성해서
    CacheItem *newCashItem = (CacheItem *)malloc(sizeof(CacheItem));

    newCashItem->data = (char *)malloc(size);
    memcpy(newCashItem->data, data, size);

    newCashItem->uri = (char *)malloc(strlen(uri) + 1);
    strcpy(newCashItem->uri, uri);

    newCashItem->size = size;

    if (cache->total_size == 0) {
        cache->head = newCashItem;
        cache->rear = newCashItem;
        newCashItem->next = NULL;
        newCashItem->prev = NULL;

    } else {
        newCashItem->next = cache->head;
        cache->head->prev = newCashItem;
        cache->head = newCashItem;
        newCashItem->prev = NULL;
    }

    cache->total_size += size;

    pthread_rwlock_unlock(&cache->rwlock);
}
