#pragma once

#include "renderer.h"
// This one is the rasteriser and is responsible for rasterisation operation 
// We will start with some simple.. Rasterizing a triangle in a simple fashion 

// This rasterizer will only work on triangles for now 

void Rasteriser(int x1, int y1, int x2, int y2, int x3, int y3);
void ScreenSpace(float x1, float y1, float x2, float y2, float x3, float y3);