#include "Renderer.h"

#include <execution>
#include <Walnut/Random.h>

namespace
{

__forceinline uint32_t ConvertToRGBA8(glm::vec4 vec4Color)
{
	vec4Color = glm::clamp(std::move(vec4Color), glm::vec4{ 0.0f }, glm::vec4{ 1.0f });

	return (static_cast<uint8_t>(vec4Color.w * 255.0f) << 24) |
		(static_cast<uint8_t>(vec4Color.z * 255.0f) << 16) |
		(static_cast<uint8_t>(vec4Color.y * 255.0f) << 8) |
		(static_cast<uint8_t>(vec4Color.x * 255.0f));
}

__forceinline uint32_t PCG_Hash(uint32_t input)
{
	uint32_t state = input * 747796405u + 2891336453u;
	uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

__forceinline float RandomFloat(uint32_t &seed)
{
	seed = PCG_Hash(seed);
	return (float)seed / (float)std::numeric_limits<uint32_t>::max();
}

__forceinline glm::vec3 InUnitSphere(uint32_t &seed)
{
	return glm::normalize(glm::vec3{
		RandomFloat(seed) * 2.0f - 1.0f,
		RandomFloat(seed) * 2.0f - 1.0f,
		RandomFloat(seed) * 2.0f - 1.0f });	
}

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

	delete[] m_pAccumulateData;
	m_pAccumulateData = new glm::vec4[width * height];

	m_frameIndex = 1;
	std::memset(m_pAccumulateData, 0, width * height * sizeof(glm::vec4));

	m_imageHorizontalIter.resize(width);
	for (uint32_t i = 0; i < width; ++i)
	{
		m_imageHorizontalIter[i] = i;
	}
	m_imageVerticallIter.resize(height);
	for (uint32_t i = 0; i < height; ++i)
	{
		m_imageVerticallIter[i] = i;
	}
}

void Renderer::ResetAccumulate()
{
	m_frameIndex = 1;
	std::memset(m_pAccumulateData, 0, m_pFinalImage->GetWidth() * m_pFinalImage->GetHeight() * sizeof(glm::vec4));
}

void Renderer::Render(const Scene &scene, const Camera &camera)
{
	m_pScene = &scene;
	m_pCamera = &camera;
	assert(m_pScene && m_pCamera && m_pFinalImage);

	const uint32_t imageWidth = m_pFinalImage->GetWidth();
	const uint32_t imageHeight = m_pFinalImage->GetHeight();

	static bool s_lastFrameAccumulateState = m_isAccumulate;
	if (!m_isAccumulate || !s_lastFrameAccumulateState)
	{
		// Clear accumulate
		m_frameIndex = 1;
		std::memset(m_pAccumulateData, 0, imageWidth * imageHeight * sizeof(glm::vec4));
	}

#define PARALLEL 1

#if PARALLEL
	std::for_each(std::execution::par, m_imageVerticallIter.begin(), m_imageVerticallIter.end(),
		[this](uint32_t y)
		{
			const uint32_t imageWidth = m_pFinalImage->GetWidth();
			for (uint32_t x = 0; x < imageWidth; ++x)
			{
				const size_t index = x + y * imageWidth;
				m_pAccumulateData[index] += PerPixel(x, y);
				m_pFinalImageData[index] = ConvertToRGBA8(m_pAccumulateData[index] / static_cast<float>(m_frameIndex));
			}
		});
#else
	for (uint32_t y = 0; y < imageHeight; ++y)
	{
		for (uint32_t x = 0; x < imageWidth; ++x)
		{
			const size_t index = x + y * imageWidth;
			m_pAccumulateData[index] += PerPixel(x, y);
			m_pFinalImageData[index] = ConvertToRGBA8(m_pAccumulateData[index] / static_cast<float>(m_frameIndex));
		}
	}
#endif

	m_pFinalImage->SetData(m_pFinalImageData);

	if (m_isAccumulate)
	{
		++m_frameIndex;
	}

	s_lastFrameAccumulateState = m_isAccumulate;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	size_t pixelIndex = x + y * m_pFinalImage->GetWidth();

	Ray ray;
	ray.Origin = m_pCamera->GetPosition();
	ray.Direction = m_pCamera->GetRayDirections()[pixelIndex];

	// per frame, per pixel, per bounce
	uint32_t seed = pixelIndex * m_frameIndex;
	glm::vec3 contribution{ 1.0f };
	glm::vec3 light{ 0.0f };
	for (uint32_t i = 0; i < m_bounces; ++i)
	{
		seed += i;

		HitPayload payload = TraceRay(ray);
		if (payload.HitDistance < 0.0f)
		{
			light += m_pScene->SkyColor * contribution;
			break;
		}

		const Sphere &sphere = m_pScene->Spheres[payload.ObjectIndex];
		const Material &material = m_pScene->Materials[sphere.MateralIndex];

		light += (material.EmissiveColor * material.EmissiveIntensity);
		contribution *= material.Albedo;

		// Avoid intersecting with current object.
		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction,
			payload.WorldNormal + material.Roughness * InUnitSphere(seed));
		// Walnut::Random::InUnitSphere()
	}

	return glm::vec4{ std::move(light), 1.0f };
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
