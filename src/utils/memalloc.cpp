// Custom memory allocator to mitigate 30% CPU time taken by vector class for allocation and deallocation
#include "../include/memalloc.h"

size_t align(size_t val, uint32_t align)
{
    if (align & (align - 1))
        return 0;
    return (val + align - 1) & ~size_t(align - 1);
}

//class MonotonicMemoryResource
//{
//    void  *buffer;
//    size_t capacity;
//    size_t size = 0;
//
//  public:
//    MonotonicMemoryResource() : buffer{nullptr}, capacity(0)
//    {
//    }
//
//    MonotonicMemoryResource(void *buffer, size_t capacity) : buffer{buffer}, capacity{capacity}
//    {
//    }
//
//    void *allocate(size_t alloc_size)
//    {
//        auto to_return = static_cast<void *>(static_cast<uint8_t *>(buffer) + size);
//        if (size + alloc_size >= capacity)
//            return nullptr;
//
//        size += alloc_size;
//        return to_return;
//    }
//
//    void *deallocate(size_t size)
//    {
//        // don't do anything
//    }
//
//    void reset()
//    {
//        size = 0;
//    }
//};
//
//template <typename T> class MemAlloc
//{
//    size_t                  max_size = 0;
//
//  public:
//    MonotonicMemoryResource *resource;
//    using value_type     = T;
//
//    constexpr MemAlloc() = default;
//    constexpr MemAlloc(MonotonicMemoryResource* resource) noexcept : resource{resource}
//    {
//    }
//
//    // give the converting constructor
//    template <typename U> constexpr MemAlloc(const MemAlloc<U> &lhs) noexcept
//    {
//        resource = lhs->resource;
//    }
//
//    [[nodiscard]] T *allocate(std::size_t n)
//    {
//        auto ptr = resource.allocate(n * sizeof(T));
//        return new (ptr) T[n];
//    }
//
//    void deallocate(T *p, std::size_t n) noexcept
//    {
//        // Don't deallocate for now
//        // head = static_cast<uint8_t *>(head) - n;
//        resource->deallocate();
//    }
//};