#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Material
{
	glm::vec3 Albedo{ 1.0f };
	float Roughness = 1.0f;
	float Metallic = 0.0f;
};

struct Sphere
{
	int MateralIndex;
	glm::vec3 Position{ 0.0f };
	float Radius = 0.5f;
};

struct Scene
{
	static constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

	glm::vec3 SkyColor{ 1.0f };
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
};
