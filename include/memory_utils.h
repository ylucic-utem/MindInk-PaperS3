/**
 * @file memory_utils.h
 * @brief Memory management utilities for MindInk PaperS3
 * 
 * Provides PSRAM buffer allocation, memory profiling, and cleanup functions.
 */

#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <M5Unified.h>

// =============================================================================
// MEMORY THRESHOLDS
// =============================================================================

// Minimum free heap before triggering cleanup (32KB)
constexpr size_t MEM_LOW_HEAP_THRESHOLD = 32768;

// Minimum free PSRAM before warning (1MB)
constexpr size_t MEM_LOW_PSRAM_THRESHOLD = 1048576;

// Maximum single allocation size for heap (avoid fragmentation)
constexpr size_t MEM_MAX_HEAP_ALLOC = 16384;

// =============================================================================
// MEMORY INFO STRUCTURE
// =============================================================================

struct MemoryInfo {
    size_t heapFree;
    size_t heapTotal;
    size_t heapLargestBlock;
    size_t psramFree;
    size_t psramTotal;
    bool isLowHeap;
    bool isLowPsram;
};

// =============================================================================
// MEMORY FUNCTIONS
// =============================================================================

/**
 * @brief Get current memory status
 * @return MemoryInfo structure with all memory stats
 */
MemoryInfo memGetInfo();

/**
 * @brief Print memory status to Serial
 */
void memPrintStatus();

/**
 * @brief Check if memory is critically low
 * @return true if heap is below threshold
 */
bool memIsLow();

/**
 * @brief Allocate buffer in PSRAM if available, else heap
 * @param size Size in bytes to allocate
 * @return Pointer to allocated buffer (nullptr on failure)
 */
void* memAllocBuffer(size_t size);

/**
 * @brief Free buffer allocated by memAllocBuffer
 * @param ptr Pointer to buffer
 */
void memFreeBuffer(void* ptr);

/**
 * @brief Force garbage collection / memory cleanup
 */
void memCleanup();

/**
 * @brief Initialize memory monitoring
 */
void memInit();

#endif // MEMORY_UTILS_H
