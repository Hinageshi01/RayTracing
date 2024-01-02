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
	m_pScene = &scene;
	m_pCamera = &camera;

	const uint32_t imageWidth = m_pFinalImage->GetWidth();
	const uint32_t imageHeight = m_pFinalImage->GetHeight();

	for (uint32_t y = 0; y < imageHeight; ++y)
	{
		for (uint32_t x = 0; x < imageWidth; ++x)
		{
			glm::vec4 color = PerPixel(x, y);
			m_pFinalImageData[x + y * imageWidth] = ConvertToRGBA8(std::move(color));
		}
	}

	m_pFinalImage->SetData(m_pFinalImageData);
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	assert(m_pScene && m_pCamera);

	Ray ray;
	ray.Origin = m_pCamera->GetPosition();
	ray.Direction = m_pCamera->GetRayDirections()[x + y * m_pFinalImage->GetWidth()];

	float weight;
	glm::vec3 finalColor{ 0.0f };
	constexpr uint32_t bounces = 2;
	for (uint32_t i = 0; i < bounces; ++i)
	{
		weight = std::pow(0.8f, i);

		HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			constexpr glm::vec3 skyColo{ 0.0f };
			finalColor += skyColo * weight;
			break;
		}

		assert(payload.ObjectIndex < m_pScene->Spheres.size());
		const Sphere &sphere = m_pScene->Spheres[payload.ObjectIndex];

		glm::vec3 lightDir = glm::normalize(glm::vec3{ -1.0f });
		float lightIntensity = glm::max(glm::dot(payload.WorldNormal, -lightDir), 0.0f);
		finalColor += sphere.Albedo * lightIntensity * weight;

		// Avoid intersecting with current object.
		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction, payload.WorldNormal);
	}

	return glm::vec4{ std::move(finalColor), 1.0f };
}

HitPayload Renderer::TraceRay(const Ray &ray)
{
	uint32_t closestSphereIndex = Scene::InvalidIndex;
	float hitDistance = std::numeric_limits<float>::max();

	for (size_t index = 0; index < m_pScene->Spheres.size(); ++index)
	{
		const Sphere &sphere = m_pScene->Spheres[index];

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
		if (closestT >= 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphereIndex = index;
		}
	}

	if (Scene::InvalidIndex == closestSphereIndex)
	{
		return Miss(ray);
	}

	return ClosestHit(ray, hitDistance, closestSphereIndex);
}

HitPayload Renderer::ClosestHit(const Ray &ray, float hitDistance, uint32_t objectIndex)
{
	HitPayload payload;
	payload.ObjectIndex = objectIndex;
	payload.HitDistance = hitDistance;

	const Sphere &closestSphere = m_pScene->Spheres[objectIndex];
	glm::vec3 origin = ray.Origin - closestSphere.Position;
	glm::vec3 hitPoint = origin + ray.Direction * hitDistance;

	payload.WorldPosition = hitPoint + closestSphere.Position;
	payload.WorldNormal = glm::normalize(std::move(hitPoint));

	return payload;
}

HitPayload Renderer::Miss(const Ray &ray)
{
	HitPayload payload;
	payload.HitDistance = -1.0f;

	return payload;
}
