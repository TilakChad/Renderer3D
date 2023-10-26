cmake_minimum_required(VERSION 3.10)

project(Renderer3D)
set(SRC "./src")
set(INCLUDE "./src/include")

if (WIN32) 
endif (WIN32) 
	set(PLATFORM_FILE ${SRC}/platform/win32_platform.cpp)
if (UNIX)
	set(PLATFORM_FILE ${SRC}/platform/linux_wayland.cpp ${SRC}/include/xdg-shell-impl.c) 
endif (UNIX)

add_executable(RenderDemo 
		${SRC}/main.cpp
		${SRC}/image/deflate.c
		${SRC}/image/PNGLoader.c
		${SRC}/geometry/objLoader.cpp
		${SRC}/Renderer/Rasterizer/parallelrenderer.cpp
		${SRC}/Renderer/Rasterizer/rasteriser.cpp
		${SRC}/Renderer/RayTracer/raytracer.cpp
		${SRC}/Renderer/renderer.cpp
		${SRC}/Renderer/texture.cpp
		${SRC}/utils/parallel_render.cpp
		${PLATFORM_FILE}
		)

include_directories(${INCLUDE})

SET(CMAKE_CXX_COMPILER "g++-13")
SET(CMAKE_CXX_FLAGS "-std=c++20 -g -march=native")

target_link_libraries(RenderDemo wayland-client dl)
