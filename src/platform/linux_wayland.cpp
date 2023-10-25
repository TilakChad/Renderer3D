#include "./linux_wayland.h"

#include <cassert>
#include <cstring>

#include <wayland-client.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h> 

static Platform        platform;
static WaylandPlatform wl; 

// mapping to existing win32 implementation
Platform GetCurrentPlatform() {
	return platform;
}

// This going to be long
#define CHECK_PTR(ptr, str) assert(ptr && str)

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define STREQ(a,b) !strcmp(a,b)

static void WLHandleRegistryGlobal(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
	eprintf("Supported global: Interface: %-25s name: %d, version: %d\n",interface, name, version);
	WaylandPlatform* state = (WaylandPlatform*) data;
	if (STREQ(interface, wl_compositor_interface.name)) {
		state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4); 
	}
	if (STREQ(interface, wl_shm_interface.name)) {
		state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
	if (STREQ(interface, xdg_wm_base_interface.name)) {
		state->xdg_wm_base = wl_registry_bind(registry, name,  &xdg_wm_base_interface, version);
	}
	
	/*
	if (STR_EQ(interface, wl_seat_interface.name)) {
		state->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
		wl_seat_add_listener(state->seat, &seat_listener, state);
	}
	*/
}

static void WLHandleRegistryGlobalRemove(void* data, struct wl_registry* registry, uint32_t name) {

}

static const struct wl_registry_listener
registry_listener = {
	.global        = WLHandleRegistryGlobal,
	.global_remove = WLHandleRegistryGlobalRemove,
};


SharedMemory CreateSharedMemory(const char* name, uint32_t size);
MemBuf       MapSharedMemory(SharedMemory* shared_mem, void* addr); 
MemBuf       AllocateBufferFromPool(MemoryPool* pool, int32_t offset, int32_t width, int32_t height, int32_t stride, int32_t format); 
				    
int main(int argc, char* argv[]) {
	wl.display = wl_display_connect(nullptr);
	CHECK_PTR(wl.display, "Wayland connection failed");

	wl.registry = wl_display_get_registry(wl.display);
	CHECK_PTR(wl.registry, "Failed to get wayland registry");

	while (wl_display_dispatch()) {
		// TODO:: Control frame rate
		RendererMainLoop(&platform);
	}

	wl_display_disconnect(wl.display); 
	return 0;
}

// Linux utilities
SharedMemory CreateSharedMemory(const char* name, uint32_t size) {
	int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (fd >= 0) {
		shm_unlink(name);
		ftruncate(fd, size);
		return (SharedMemory) {
			.shm_name = name,
			.fd       = fd,
			.size     = size
		}; 
	}
	perror("shm_open() failed"); 
	assert(false);
}

MemBuf       MapSharedMemory(SharedMemory* shared_mem, void* addr) {
	MemBuf buf;
	buf.size = shared_mem->size;
	buf.ptr  = mmap(addr, buf.size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem->fd, 0);
	assert(buf.ptr != MAP_FAILED); 
	return buf; 
}

MemBuf       AllocateBufferFromPool(MemoryPool* pool, int32_t offset, int32_t width, int32_t height, int32_t stride, int32_t format) {
	// Assert that the pool have enough free space
	uint32_t required_mem = height * stride;
	assert((offset + required_mem) < pool->buffer_range.size);
	struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool->shm_pool, offset,  width, height, stride, format);
	wl_buffer_add_listener(buffer, &buffer_listener, nullptr);
	return (MemBuf) {.ptr = (uint8_t*)pool->buffer_range.ptr + offset, .size = required_mem, .wl_buffer_handle = buffer}; 
}
