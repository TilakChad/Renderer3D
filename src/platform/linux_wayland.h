#include "../include/platform.h"

struct wl_display;
struct wl_buffer;
struct wl_shm_pool;
struct wl_shm;
struct wl_seat; 
struct wl_surface;
struct wl_callback;

struct xdg_wm_base;
struct wl_buffer;

struct SharedMemory {
       	uint32_t    size;
	int         fd;
	const char* shm_name;
};

struct MemBuf {
	uint32_t size;
	void     *ptr;
	
	struct wl_buffer* wl_buffer_handle;
};

struct MemoryPool {
	struct wl_shm      *shm; 
	struct wl_shm_pool *shm_pool;
	struct MemBuf      alloc_range;
}; 

struct WaylandPlatform {
	struct wl_registry *registry; 
	struct wl_display  *display;
	struct wl_buffer   *buffer;
	struct wl_seat     *seat; 

	// XDG Extensions
	struct xdg_wm_base *wm_base;
	struct wl_surface  *surface;
	struct wl_callback *callback;

	struct SharedMemory shared_memory_info;
	struct MemBuf       mapped_range;
};

