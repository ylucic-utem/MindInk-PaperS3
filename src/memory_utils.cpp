/**
 * @file memory_utils.cpp
 * @brief Memory management utilities implementation
 */

#include "memory_utils.h"
#include <esp_heap_caps.h>

// =============================================================================
// MEMORY INFO
// =============================================================================

MemoryInfo memGetInfo() {
    MemoryInfo info;
    
    // Heap stats
    info.heapFree = ESP.getFreeHeap();
    info.heapTotal = ESP.getHeapSize();
    info.heapLargestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    
    // PSRAM stats
    info.psramFree = ESP.getFreePsram();
    info.psramTotal = ESP.getPsramSize();
    
    // Thresholds
    info.isLowHeap = info.heapFree < MEM_LOW_HEAP_THRESHOLD;
    info.isLowPsram = info.psramFree < MEM_LOW_PSRAM_THRESHOLD && info.psramTotal > 0;
    
    return info;
}

void memPrintStatus() {
    MemoryInfo info = memGetInfo();
    
    Serial.println("[MEMORY] === Memory Status ===");
    Serial.printf("[MEMORY] Heap:  %d / %d KB (largest block: %d KB)\n", 
        info.heapFree / 1024, info.heapTotal / 1024, info.heapLargestBlock / 1024);
    Serial.printf("[MEMORY] PSRAM: %d / %d KB\n",
        info.psramFree / 1024, info.psramTotal / 1024);
    
    if (info.isLowHeap) {
        Serial.println("[MEMORY] WARNING: Low heap memory!");
    }
    if (info.isLowPsram) {
        Serial.println("[MEMORY] WARNING: Low PSRAM!");
    }
}

bool memIsLow() {
    return ESP.getFreeHeap() < MEM_LOW_HEAP_THRESHOLD;
}

// =============================================================================
// BUFFER ALLOCATION
// =============================================================================

void* memAllocBuffer(size_t size) {
    void* ptr = nullptr;
    
    // Try PSRAM first for large allocations
    if (size > MEM_MAX_HEAP_ALLOC && ESP.getPsramSize() > 0) {
        ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (ptr) {
            Serial.printf("[MEMORY] Allocated %d bytes in PSRAM\n", size);
            return ptr;
        }
    }
    
    // Fall back to heap for small allocations or if PSRAM failed
    ptr = malloc(size);
    if (ptr) {
        Serial.printf("[MEMORY] Allocated %d bytes in heap\n", size);
    } else {
        Serial.printf("[MEMORY] Failed to allocate %d bytes!\n", size);
    }
    
    return ptr;
}

void memFreeBuffer(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// =============================================================================
// CLEANUP
// =============================================================================

void memCleanup() {
    Serial.println("[MEMORY] Running cleanup...");
    
    // Force garbage collection by freeing any cached resources
    // Note: In ESP32/Arduino, there's no explicit GC, but we can hint
    
    // Print status after cleanup
    memPrintStatus();
}

void memInit() {
    Serial.println("[MEMORY] Memory utilities initialized");
    memPrintStatus();
    
    // Configure heap monitoring
    // heap_caps_register_failed_alloc_callback() could be used here
}
