#include "cashier.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

struct cashier *cashier_init(struct cache_config config)
{
    // Allocate memory for the cashier struct
    struct cashier *cache = malloc(sizeof(struct cashier));
    if (cache == NULL)
    {
        // Return NULL on memory allocation failure
        return NULL;
    }

    // Set the cache configuration
    cache->config = config;
    cache->size = config.lines * config.line_size;

    // Calculate the number of bits for tag, index, and offset
    cache->tag_bits = config.address_bits - (config.ways + config.line_size);
    cache->index_bits = config.address_bits - cache->tag_bits - config.line_size;
    cache->offset_bits = config.line_size;

    // Create bit masks for tag, index, and offset
    cache->tag_mask = (1ULL << cache->tag_bits) - 1;
    cache->index_mask = ((1ULL << cache->index_bits) - 1) << cache->offset_bits;
    cache->offset_mask = (1ULL << cache->offset_bits) - 1;

    // Allocate memory for cache lines
    cache->lines = malloc(config.lines * sizeof(struct cache_line));
    if (cache->lines == NULL)
    {
        // Free previously allocated memory and return NULL on memory allocation failure
        free(cache);
        return NULL;
    }

    // Initialize cache lines
    for (uint64_t i = 0; i < config.lines; i++)
    {
        struct cache_line *line = &(cache->lines[i]);
        line->valid = false;
        line->dirty = false;
        line->tag = 0;
        line->last_access = 0;
        line->data = malloc(config.line_size * sizeof(uint8_t));
        if (line->data == NULL)
        {
            // Free previously allocated memory and return NULL on memory allocation failure
            cashier_release(cache);
            return NULL;
        }
    }

    return cache;
}

void cashier_release(struct cashier *cache)
{
    if (cache == NULL)
    {
        return;
    }

    // Write back dirty lines
    for (uint64_t i = 0; i < cache->config.lines; i++)
    {
        struct cache_line *line = &(cache->lines[i]);
        if (line->valid && line->dirty)
        {
            // Write the cache line data back to memory
            uint64_t addr = (line->tag << (cache->index_bits + cache->offset_bits)) |
                            (i << cache->offset_bits);
            for (uint64_t j = 0; j < cache->config.line_size; j++)
            {
                mem_write(addr + j, line->data[j]);
            }
        }
    }

    // Free memory for cache lines
    for (uint64_t i = 0; i < cache->config.lines; i++)
    {
        free(cache->lines[i].data);
    }

    // Free memory for the cache simulator
    free(cache->lines);
    free(cache);
}

bool cashier_read(struct cashier *cache, uint64_t addr, uint8_t *byte)
{
    if (cache == NULL || byte == NULL)
    {
        return false;
    }

    // Extract tag, index, and offset from the address
    uint64_t tag = (addr & cache->tag_mask) >> (cache->index_bits + cache->offset_bits);
    uint64_t index = (addr & cache->index_mask) >> cache->offset_bits;
    uint64_t offset = addr & cache->offset_mask;

    // Find the cache line within the set
    uint64_t start_line = index * cache->config.ways;
    uint64_t end_line = start_line + cache->config.ways;
    uint64_t lru_line = start_line;
    uint64_t lru_access = cache->lines[start_line].last_access;

    // Check if the data is in the cache
    for (uint64_t i = start_line; i < end_line; i++)
    {
        struct cache_line *line = &(cache->lines[i]);
        if (line->valid && line->tag == tag)
        {
            // Cache hit
            *byte = line->data[offset];
            line->last_access = get_timestamp();
            return true;
        }

        // Find the least recently used line
        if (line->last_access < lru_access)
        {
            lru_access = line->last_access;
            lru_line = i;
        }
    }

    // Cache miss
    struct cache_line *line = &(cache->lines[lru_line]);
    if (line->valid && line->dirty)
    {
        // Write the evicted cache line data back to memory
        uint64_t evicted_addr = (line->tag << (cache->index_bits + cache->offset_bits)) |
                                (lru_line << cache->offset_bits);
        for (uint64_t j = 0; j < cache->config.line_size; j++)
        {
            mem_write(evicted_addr + j, line->data[j]);
        }
    }

    // Read the cache line from memory
    uint64_t cache_addr = (tag << (cache->index_bits + cache->offset_bits)) |
                          (index << cache->offset_bits);
    for (uint64_t j = 0; j < cache->config.line_size; j++)
    {
        line->data[j] = mem_read(cache_addr + j);
    }

    // Update cache line metadata
    line->valid = true;
    line->dirty = false;
    line->tag = tag;
    line->last_access = get_timestamp();

    // Return the requested byte
    *byte = line->data[offset];
    return false;
}

bool cashier_write(struct cashier *cache, uint64_t addr, uint8_t byte)
{
    if (cache == NULL)
    {
        return false;
    }

    // Extract tag, index, and offset from the address
    uint64_t tag = (addr & cache->tag_mask) >> (cache->index_bits + cache->offset_bits);
    uint64_t index = (addr & cache->index_mask) >> cache->offset_bits;
    uint64_t offset = addr & cache->offset_mask;

    // Find the cache line within the set
    uint64_t start_line = index * cache->config.ways;
    uint64_t end_line = start_line + cache->config.ways;
    uint64_t lru_line = start_line;
    uint64_t lru_access = cache->lines[start_line].last_access;

    // Check if the data is in the cache
    for (uint64_t i = start_line; i < end_line; i++)
    {
        struct cache_line *line = &(cache->lines[i]);
        if (line->valid && line->tag == tag)
        {
            // Cache hit
            line->data[offset] = byte;
            line->last_access = get_timestamp();
            line->dirty = true;
            return true;
        }

        // Find the least recently used line
        if (line->last_access < lru_access)
        {
            lru_access = line->last_access;
            lru_line = i;
        }
    }

    // Cache miss
    struct cache_line *line = &(cache->lines[lru_line]);
    if (line->valid && line->dirty)
    {
        // Write the evicted cache line data back to memory
        uint64_t evicted_addr = (line->tag << (cache->index_bits + cache->offset_bits)) |
                                (lru_line << cache->offset_bits);
        for (uint64_t j = 0; j < cache->config.line_size; j++)
        {
            mem_write(evicted_addr + j, line->data[j]);
        }
    }

    // Read the cache line from memory
    uint64_t cache_addr = (tag << (cache->index_bits + cache->offset_bits)) |
                          (index << cache->offset_bits);
    for (uint64_t j = 0; j < cache->config.line_size; j++)
    {
        line->data[j] = mem_read(cache_addr + j);
    }

    // Update cache line metadata
    line->valid = true;
    line->dirty = true;
    line->tag = tag;
    line->last_access = get_timestamp();

    // Write the byte to the cache line
    line->data[offset] = byte;
    return false;
}
