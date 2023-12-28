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

void Renderer::Render(const Scene &scene, const Camera &camera)
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

			glm::vec4 color = TraceRay(scene, ray);
			m_pFinalImageData[x + y * imageWidth] = ConvertToRGBA8(std::move(color));
		}
	}

	m_pFinalImage->SetData(m_pFinalImageData);
}

glm::vec4 Renderer::TraceRay(const Scene &scene, const Ray &ray)
{
	if (scene.Spaeres.empty())
	{
		return glm::vec4{ 0.0f, 0.0f , 0.0f, 1.0f };
	}

	const Sphere *pClosestSphere = nullptr;;
	float hitDistance = std::numeric_limits<float>::max();
	for (const auto &sphere : scene.Spaeres)
	{
		glm::vec3 origin = ray.Origin - sphere.Position;
		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
		{
			continue;
		}

		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT < hitDistance)
		{
			hitDistance = closestT;
			pClosestSphere = &sphere;
		}
	}

	if (!pClosestSphere)
	{
		return glm::vec4{ 0.0f, 0.0f , 0.0f, 1.0f };
	}


	glm::vec3 origin = ray.Origin - pClosestSphere->Position;
	glm::vec3 hitPoint = origin + ray.Direction * hitDistance;
	glm::vec3 normal = glm::normalize(hitPoint);
	glm::vec3 lightDir = glm::normalize(glm::vec3{ -1.0f });
	float lightIntensity = glm::max(glm::dot(normal, -lightDir), 0.0f);

	glm::vec3 sphereColor = pClosestSphere->Albedo;
	sphereColor *= lightIntensity;
	return glm::vec4{ std::move(sphereColor), 1.0f };
}
