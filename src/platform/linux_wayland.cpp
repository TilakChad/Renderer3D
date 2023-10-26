#include "./linux_wayland.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <wayland-client.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "../include/xdg-shell.h"

#define CHECK_PTR(ptr, str) assert(ptr && str)

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define STREQ(a,b) !strcmp(a,b)


#define Bytes(x)     (x) 
#define Kilobytes(x) Bytes(1024 * x)
#define Megabytes(x) Kilobytes(1024 * x)

#define SHMNAME "/renderer3d"

SharedMemory CreateSharedMemory(const char* name, uint32_t size);
MemBuf       MapSharedMemory(SharedMemory* shared_mem, void* addr); 
MemBuf       AllocateBufferFromPool(MemoryPool* pool, int32_t offset, int32_t width, int32_t height, int32_t stride, int32_t format);

static void WLHandleRegistryGlobal(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version); 
static void WLHandleRegistryGlobalRemove(void* data, struct wl_registry* registry, uint32_t name) {

}

// Ping pong for WMBase
static void XDGWMBasePing(void* data, struct xdg_wm_base* wm_base, uint32_t serial) {
	xdg_wm_base_pong(wm_base, serial);
}

void DrawCheckBoard(MemBuf buffer, uint32_t width, uint32_t height); 

static void XDGSurfaceConfigure(void* data, struct xdg_surface* surface, uint32_t serial)  {
	PlatformData* state = static_cast<PlatformData*>(data);
	// We not going to destroy the buffer, so

	 auto buf = state->wl.surface_buf;
	 // DrawCheckBoard(state->wl.surface_buf, 800, 600);
	
	 wl_surface_attach(state->wl.surface, buf.wl_buffer_handle, 0, 0);
	 wl_surface_damage(state->wl.surface, 0, 0, INT32_MAX, INT32_MAX); 
	 wl_surface_commit(state->wl.surface); 

	 xdg_surface_ack_configure(surface, serial);
	 eprintf("%s\n","Commiting");
}

static const struct xdg_wm_base_listener
wm_base_listener = {
	.ping = XDGWMBasePing
};

static const struct xdg_surface_listener
surface_listener = {
	.configure = XDGSurfaceConfigure
}; 

static const struct wl_registry_listener
registry_listener = {
	.global        = WLHandleRegistryGlobal,
	.global_remove = WLHandleRegistryGlobalRemove,
};


static PlatformData platform;
static void SwapBuffers();

static void frame_callback(void* data, struct wl_callback* callback, uint32_t callback_data);

static const struct wl_callback_listener
callback_listener = {
	.done = frame_callback
};

static void wl_buffer_release(void* data, struct wl_buffer* buffer) {
	PlatformData* state = static_cast<PlatformData*>(data);
	state->wl.frame_lock.store(true);
	// eprintf("%s\n", "Releasing frame\n");
}

static const struct wl_buffer_listener buffer_listener = {
	.release = wl_buffer_release
}; 

static void frame_callback(void* data, struct wl_callback* callback, uint32_t callback_data) {
	PlatformData* state = static_cast<PlatformData*>(data);

	wl_callback_destroy(callback);

	struct wl_callback* ncb = wl_surface_frame(state->wl.surface);
	wl_callback_add_listener(ncb, &callback_listener, state); 

	// update the delta time in seconds
	static bool first_frame = true;
	static uint32_t prev_data;
	if (first_frame) {
		prev_data   = callback_data;
		first_frame = false; 
	}

	state->data.deltaTime = (callback_data - prev_data) / 1500.0f ;
	prev_data            = callback_data;

	RendererMainLoop(&platform.data);
	
	wl_surface_attach(state->wl.surface, state->wl.surface_buf.wl_buffer_handle, 0, 0);
	wl_surface_damage(state->wl.surface, 0,0, INT32_MAX,INT32_MAX); 
	wl_surface_commit(state->wl.surface);
}

int main(int argc, char* argv[]) {
	// For Shadow Mapping
	constexpr int width  = 1280;
	constexpr int height = 720;
	struct WaylandPlatform& wl = platform.wl; 

	wl.display = wl_display_connect(nullptr);
	CHECK_PTR(wl.display, "Wayland connection failed");

	wl.registry = wl_display_get_registry(wl.display);
	CHECK_PTR(wl.registry, "Failed to connect to wayland registry");

	wl_registry_add_listener(wl.registry, &registry_listener, &platform);
	wl_display_roundtrip(wl.display);

	CHECK_PTR(wl.compositor, "Failed to find wl compositor");
	wl.surface = wl_compositor_create_surface(wl.compositor);
	
	CHECK_PTR(wl.xdg.wm_base, "Failed to find xdg_wm_base");
	xdg_wm_base_add_listener(wl.xdg.wm_base, &wm_base_listener, &platform);


	wl.shared_memory_info = CreateSharedMemory(SHMNAME, Megabytes(4));
	wl.pool.mapped_range  = MapSharedMemory(&wl.shared_memory_info, nullptr);

	// Create memory pool
	assert(wl.pool.shm); 
	wl.pool.shm_pool = wl_shm_create_pool(wl.pool.shm,wl.shared_memory_info.fd, wl.shared_memory_info.size); 

	// allocate a buffer from the memory pool
	wl.surface_buf   =  AllocateBufferFromPool(&wl.pool, 0, width, height, width * 4, WL_SHM_FORMAT_XRGB8888);
	wl_buffer_add_listener(wl.surface_buf.wl_buffer_handle, &buffer_listener, &platform);
	memset(wl.surface_buf.ptr, wl.surface_buf.size, 0xFF);

	
	wl.xdg.surface = xdg_wm_base_get_xdg_surface(wl.xdg.wm_base, wl.surface);
	xdg_surface_add_listener(wl.xdg.surface, &surface_listener, &platform);  

	CHECK_PTR(wl.xdg.surface,"Failed to create xdg surface");
	wl.xdg.toplevel = xdg_surface_get_toplevel(wl.xdg.surface); 


	
	platform.data.shadowMap.width  = width;
	platform.data.shadowMap.height = height;
	platform.data.shadowMap.buffer = static_cast<float*>(malloc(sizeof(float) * platform.data.shadowMap.width * platform.data.shadowMap.height));

	// For main Color Buffer
	platform.data.colorBuffer.width      = width;
	platform.data.colorBuffer.height     = height;
	platform.data.colorBuffer.noChannels = 4;
	platform.data.colorBuffer.buffer = static_cast<uint8_t*>(wl.surface_buf.ptr); 
	
	wl_surface_attach(wl.surface, wl.surface_buf.wl_buffer_handle, 0, 0);
	wl_surface_commit(wl.surface);

	// Window Dimension 
	platform.data.width  = width;
	platform.data.height = height;

	platform.data.zBuffer.width  = width;
	platform.data.zBuffer.height = height;
	platform.data.zBuffer.buffer =  static_cast<float*>(malloc(sizeof(float) * platform.data.shadowMap.width * platform.data.shadowMap.height));

	platform.data.SwapBuffer = ::SwapBuffers;
	
	struct wl_callback* cb = wl_surface_frame(wl.surface);
	wl_callback_add_listener(cb, &callback_listener, &platform);

	while (wl_display_dispatch(wl.display)) {
	}

	// while(true) {
	// 	
	//      wl_display_dispatch_pending(wl.display); 
	// 	platform.wl.frame_lock.store(false);

	// 	RendererMainLoop(&platform.data);
	// 	// Poor man's synchronization fence
	// 	while (!platform.wl.frame_lock) {
	// 		wl_display_dispatch(wl.display);
	// 	}
	// }

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
			.size     = size,
			.fd       = fd,
			.shm_name = name,
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
	assert((offset + required_mem) < pool->mapped_range.size);
	struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool->shm_pool, offset,  width, height, stride, format);
	// wl_buffer_add_listener(buffer, &buffer_listener, nullptr);
	return (MemBuf) {
		.size = required_mem,
		.ptr = (uint8_t*)pool->mapped_range.ptr + offset,
		.wl_buffer_handle = buffer
	}; 
}


static void WLHandleRegistryGlobal(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
	PlatformData* state = (PlatformData*) data;
	if (STREQ(interface, wl_compositor_interface.name)) {
		state->wl.compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 4)); 
	}
	if (STREQ(interface, wl_shm_interface.name)) {
		state->wl.pool.shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
	}
	if (STREQ(interface, xdg_wm_base_interface.name)) {
		state->wl.xdg.wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(registry, name,  &xdg_wm_base_interface, version));
	}
	
	/*
	if (STREQ(interface, wl_seat_interface.name)) {
		state->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
		wl_seat_add_listener(state->seat, &seat_listener, state);
	}
	*/
}


// mapping to existing win32 implementation
Platform GetCurrentPlatform() {
	return platform.data;
}

void SwapBuffers() {
	//  MemBuf buf = AllocateBufferFromPool(&platform.wl.pool, 0, 800, 600, 800 * 4, WL_SHM_FORMAT_XRGB8888);
	//  Memset(buf.ptr, 0xFF, buf.size);

	// DrawCheckBoard(buf, 800, 600);
	
	// wl_surface_attach(platform.wl.surface, buf.wl_buffer_handle, 0, 0);  
	// wl_surface_commit(platform.wl.surface); 
}

void DrawCheckBoard(MemBuf buffer, uint32_t width, uint32_t height) {
	// Assuming four components
	const uint32_t interval = 100;
	const uint32_t stride   = width * 4;

	static int offset = 0;

	offset = offset + 1; 
	uint8_t* row = static_cast<uint8_t*>(buffer.ptr);
	
	for (int j = 0; j < height; j += 1) {		
		for (int i = 0; i < width; i += 1) {
			if (((i + offset) / interval + (j + offset) / interval) % 2) {
				(row + i * 4)[0] = 0xFF;  
				(row + i * 4)[1] = 0xFF;
				(row + i * 4)[2] = 0xFF;  
				(row + i * 4)[3] = 0xFF;  
			}
			else {
				(row + i * 4)[0] = 0x00;
				(row + i * 4)[1] = 0x00;
				(row + i * 4)[2] = 0x00;  
				(row + i * 4)[3] = 0xFF;  
			}
		}
		row  = row + stride; 
	}
}
