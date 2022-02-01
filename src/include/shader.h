#pragma once

enum class Shading
{
	Flat,
	Smooth,
	Phong,
	ShadowMap, 
};

inline constexpr Shading shading = Shading::Phong;