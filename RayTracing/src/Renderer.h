#pragma once

#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

#include <glm/glm.hpp>
#include <memory>
#include <Walnut/Image.h>

struct HitPayload
{
	uint32_t ObjectIndex = Scene::InvalidIndex;

	float HitDistance = -1.0f;
	glm::vec3 WorldPosition;
	glm::vec3 WorldNormal;
};

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;

	void Render(const Scene &scene, const Camera &camera);
	void OnResize(uint32_t width, uint32_t height);

	void ResetAccumulate();
	void StartAccumulate() { m_isAccumulate = true; }
	void StopAccumulate() { m_isAccumulate = false; }

	void SetBounces(int bounces) { m_bounces = bounces; }
	int& GetBounces() { return m_bounces; }
	const int & GetBounces() const { return m_bounces; }

	std::shared_ptr<Walnut::Image> GetFinalImage() { return m_pFinalImage; }
	const std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_pFinalImage; }

private:
	// Gen Ray
	glm::vec4 PerPixel(uint32_t x, uint32_t y);
	
	HitPayload TraceRay(const Ray &ray);
	HitPayload ClosestHit(const Ray &ray, float hitDistance, uint32_t objectIndex);
	HitPayload Miss(const Ray &ray);

	int m_bounces = 1;
	bool m_isAccumulate = true;
	uint32_t m_frameIndex = 1;

	const Scene *m_pScene = nullptr;
	const Camera *m_pCamera = nullptr;

	uint32_t *m_pFinalImageData = nullptr;
	glm::vec4 *m_pAccumulateData = nullptr;

	std::shared_ptr<Walnut::Image> m_pFinalImage;
};
