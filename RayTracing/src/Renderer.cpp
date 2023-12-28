#include "Renderer.h"

#include <Walnut/Random.h>

namespace
{

uint32_t ConvertToRGBA8(glm::vec4 vec4Color)
{
	vec4Color = glm::clamp(std::move(vec4Color), glm::vec4{ 0.0f }, glm::vec4{ 1.0f });

	return (static_cast<uint8_t>(vec4Color.w * 255.0f) << 24) |
		(static_cast<uint8_t>(vec4Color.z * 255.0f) << 16) |
		(static_cast<uint8_t>(vec4Color.y * 255.0f) << 8) |
		(static_cast<uint8_t>(vec4Color.x * 255.0f));
}

}

Renderer::~Renderer()
{
	delete[] m_pFinalImageData;
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_pFinalImage && m_pFinalImage->GetWidth() == width && m_pFinalImage->GetHeight() == height)
	{
		return;
	}

	if (m_pFinalImage)
	{
		m_pFinalImage->Resize(width, height);
	}
	else
	{
		m_pFinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_pFinalImageData;
	m_pFinalImageData = new uint32_t[width * height];
}

void Renderer::Render(const Camera &camera)
{
	const uint32_t imageWidth = m_pFinalImage->GetWidth();
	const uint32_t imageHeight = m_pFinalImage->GetHeight();

	Ray ray;
	ray.Origin = camera.GetPosition();

	for (size_t y = 0; y < imageHeight; ++y)
	{
		for (size_t x = 0; x < imageWidth; ++x)
		{
			ray.Direction = camera.GetRayDirections()[x + y * imageWidth];

			glm::vec4 color = TraceRay(ray);
			m_pFinalImageData[x + y * imageWidth] = ConvertToRGBA8(std::move(color));
		}
	}

	m_pFinalImage->SetData(m_pFinalImageData);
}

glm::vec4 Renderer::TraceRay(const Ray &ray)
{
	constexpr float radius = 0.5f;
	float a = glm::dot(ray.Direction, ray.Direction);
	float b = 2.0f * glm::dot(ray.Origin, ray.Direction);
	float c = glm::dot(ray.Origin, ray.Origin) - radius * radius;

	float discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0f)
	{
		return glm::vec4{ 0.0f, 0.0f , 0.0f, 1.0f };
	}

	float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
	glm::vec3 hitPoint = ray.Origin + ray.Direction * closestT;
	glm::vec3 normal = glm::normalize(hitPoint);
	glm::vec3 lightDir = glm::normalize(glm::vec3{ -1.0f });
	float lightIntensity = glm::max(glm::dot(normal, -lightDir), 0.0f);

	glm::vec3 sphereColor{ 1.0f };
	sphereColor *= lightIntensity;
	return glm::vec4{ std::move(sphereColor), 1.0f};
}
