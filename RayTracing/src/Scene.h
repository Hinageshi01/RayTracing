#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Sphere
{
	glm::vec3 Albedo{ 1.0f };
	glm::vec3 Position{ 0.0f };
	float Radius = 0.5f;
};

struct Scene
{
	std::vector<Sphere> Spaeres;
};
