#include "../include/platform.h"
#include <atomic>

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
	struct MemBuf       mapped_range;
}; 

struct xdg {
	struct xdg_wm_base  *wm_base;
	struct xdg_toplevel *toplevel;
	struct xdg_surface  *surface;
};

struct WaylandPlatform {
	struct wl_registry   *registry; 
	struct wl_display    *display;
	struct wl_compositor *compositor; 
	struct wl_buffer     *buffer;
	struct wl_seat       *seat; 
	struct wl_callback  *callback;

	struct wl_surface    *surface;
	struct MemBuf        surface_buf; 

	struct xdg          xdg;

	struct SharedMemory shared_memory_info;
	struct MemoryPool   pool;

	std::atomic<bool>    frame_lock = false; 
};


struct PlatformData {
	Platform        data;
	WaylandPlatform wl;
};

