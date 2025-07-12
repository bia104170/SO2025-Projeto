#ifndef CACHE
#define CACHE

#include "servidor_config.h"
#include <glib.h>

enum Cache_Tipo {NO_CACHE, CACHE_FIFO, CACHE_TIME ,CACHE_FREQUENCY};

typedef struct cache_entry {

    TipoPedido tipo;
    char titulo[200];
    char autores[200];
    int year;
    char path[64];
    int index;
    int index_inserido;
    char keyword[200];
    int n_linhas;
    GArray* indices;

} CacheEntry;

typedef struct cache_frequency {

    GArray* lista;

} CacheFrequency;

typedef struct cache_fifo {
    GQueue* queue;
} CacheFIFO;

typedef struct cache_time_entry {
    CacheEntry* entry;
    time_t timestamp;
} CacheTimeEntry;

typedef struct cache_time {
    GQueue* queue;  // rever isto pq achp que consigo uma melhor abordagem
} CacheTime;

typedef struct cache {

    enum Cache_Tipo tipo_cache;
    int max_entries;
    CacheFIFO cache_fifo;
    CacheTime cache_time;
    CacheFrequency cache_frequency;

} Cache;

int compare_cache_entry_message(CacheEntry* cache_entry,mensagem msg);
Cache createCache(enum Cache_Tipo tipo, int max_entries);
CacheEntry* cache_get(Cache* cache,mensagem msg);
void cache_insert(Cache* cache,CacheEntry* entry);
void cache_filter(Cache* cache,CacheEntry* entry);
void printCacheEntry(CacheEntry* entry);
void freeCacheEntry(CacheEntry* entry);

#endif