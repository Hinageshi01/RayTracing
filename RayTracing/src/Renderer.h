#pragma once

#include "Camera.h"
#include "Ray.h"

#include <glm/glm.hpp>
#include <memory>
#include <Walnut/Image.h>

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	void Render(const Camera &camera);
	void OnResize(uint32_t width, uint32_t height);

	std::shared_ptr<Walnut::Image> GetFinalImage() { return m_pFinalImage; }
	const std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_pFinalImage; }

private:
	glm::vec4 TraceRay(const Ray &ray);

	uint32_t *m_pFinalImageData = nullptr;
	std::shared_ptr<Walnut::Image> m_pFinalImage;
};
