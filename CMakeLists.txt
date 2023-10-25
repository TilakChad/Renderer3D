cmake_minimum_required(VERSION 3.10)

project(Renderer3D)
set(SRC "./src")
set(INCLUDE "./src/include")

add_executable(RenderDemo 
		${SRC}/main.cpp
		${SRC}/image/deflate.c
		${SRC}/image/PNGLoader.c
		${SRC}/geometry/objLoader.cpp
		${SRC}/platform/linux_wayland.cpp
		${SRC}/Renderer/Rasterizer/parallelrenderer.cpp
		${SRC}/Renderer/Rasterizer/rasteriser.cpp
		${SRC}/Renderer/RayTracer/raytracer.cpp
		${SRC}/Renderer/renderer.cpp
		${SRC}/Renderer/texture.cpp
		${SRC}/utils/parallel_render.cpp

		)

include_directories(${INCLUDE})

SET(CMAKE_CXX_COMPILER "g++-13")
SET(CMAKE_CXX_FLAGS "-std=c++20 -O1 -march=native")

target_link_libraries(RenderDemo wayland-client dl)
