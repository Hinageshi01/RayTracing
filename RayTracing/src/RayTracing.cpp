#include "Camera.h"
#include "Renderer.h"
#include "Scene.h"

#include <glm/gtc/type_ptr.hpp>
#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Image.h>
#include <Walnut/Timer.h>

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer() : m_camera(45.0f, 0.01f, 1000.0f)
	{
		Sphere sphere;
		Material material;

		// Grund
		material.Albedo = glm::vec3{ 0.5f, 0.5f, 0.5f };
		material.Roughness = 1.0f;
		material.Metallic = 0.0f;
		m_scene.Materials.emplace_back(std::move(material));

		sphere.MateralIndex = m_scene.Materials.size() - 1;
		sphere.Position = glm::vec3{ 0.0f, -200.0f, 0.0f };
		sphere.Radius = 200.0f;
		m_scene.Spheres.emplace_back(std::move(sphere));

		// Pink Sphere
		material.Albedo = glm::vec3{ 1.0f, 0.3f, 0.9f };
		material.Roughness = 1.0f;
		material.Metallic = 0.0f;
		m_scene.Materials.emplace_back(std::move(material));

		sphere.MateralIndex = m_scene.Materials.size() - 1;
		sphere.Position = glm::vec3{ 0.0f, 1.0f, 0.0f };
		sphere.Radius = 1.0f;
		m_scene.Spheres.emplace_back(std::move(sphere));

		// Mirror Sphere
		material.Albedo = glm::vec3{ 1.0f };
		material.Roughness = 0.1f;
		material.Metallic = 0.0f;
		m_scene.Materials.emplace_back(std::move(material));

		sphere.MateralIndex = m_scene.Materials.size() - 1;
		sphere.Position = glm::vec3{ -3.0f, 1.0f, 0.0f };
		sphere.Radius = 1.0f;
		m_scene.Spheres.emplace_back(std::move(sphere));

		// Emissive Sphere
		material.Albedo = glm::vec3{ 1.0f };
		material.Roughness = 1.0f;
		material.Metallic = 0.0f;
		material.EmissiveColor = glm::vec3{ 1.0f, 0.4f, 0.1f };
		material.EmissiveIntensity = 2.0f;
		m_scene.Materials.emplace_back(std::move(material));

		sphere.MateralIndex = m_scene.Materials.size() - 1;
		sphere.Position = glm::vec3{ 30.0f, 20.0f, -30.0f };
		sphere.Radius = 30.0f;
		m_scene.Spheres.emplace_back(std::move(sphere));

		// Sky
		m_scene.SkyColor = glm::vec3{ 0.37f, 0.53f, 1.0f };

		m_renderer.SetBounces(8);
	}

	virtual void OnUpdate(float ts) override
	{
		if (m_camera.OnUpdate(ts))
		{
			m_renderer.StopAccumulate();
		}
		else
		{
			m_renderer.StartAccumulate();
		}
	}

	virtual void OnUIRender() override
	{
		bool needReset = false;
		auto CheckReset = [&needReset](bool reset)
		{
			if (reset)
			{
				needReset = true;
			}
		};

		// Infos
		ImGui::Begin("Setting");
		ImGui::Text("Last Frame: %.3fms", m_lastFrameTime);
		ImGui::Separator();
		CheckReset(ImGui::DragInt("Bounces", &m_renderer.GetBounces(), 0.05f, 0, 16));
		ImGui::End();

		// Object List
		ImGui::Begin("Scene");
		CheckReset(ImGui::ColorEdit3("Albedo", glm::value_ptr(m_scene.SkyColor)));
		ImGui::Separator();
		for (size_t i = 0; i < m_scene.Spheres.size(); ++i)
		{
			ImGui::PushID(i);
			Sphere &sphere = m_scene.Spheres[i];
			CheckReset(ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f));
			CheckReset(ImGui::DragFloat("Radius", &sphere.Radius, 0.1f, 0.0f, 100.0f));
			CheckReset(ImGui::DragInt("Material", &sphere.MateralIndex, 1, 0, (int)m_scene.Materials.size() - 1));
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();

		// Material List
		ImGui::Begin("Material");
		for (size_t i = 0; i < m_scene.Materials.size(); ++i)
		{
			Material& material = m_scene.Materials[i];
			ImGui::PushID(i);
			CheckReset(ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo)));
			CheckReset(ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f));
			CheckReset(ImGui::DragFloat("Metallic", &material.Metallic, 0.01f, 0.0f, 1.0f));
			CheckReset(ImGui::ColorEdit3("Emissive Color", glm::value_ptr(material.EmissiveColor)));
			CheckReset(ImGui::DragFloat("Emissive Intensity", &material.EmissiveIntensity, 0.1f, 0.0f, 100.0f));
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::End();

		// Scene View
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("View Port");
		m_viewportWidth = ImGui::GetContentRegionAvail().x;
		m_viewportHeight = ImGui::GetContentRegionAvail().y;
		if (auto pImage = m_renderer.GetFinalImage(); pImage)
		{
			ImGui::Image(pImage->GetDescriptorSet(),
				ImVec2{ (float)pImage->GetWidth(), (float)pImage->GetHeight() },
				ImVec2{ 0.0f, 1.0f }, ImVec2{ 1.0f, 0.0f });
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if (needReset)
		{
			m_renderer.ResetAccumulate();
		}

		Render();
	}

	void Render()
	{

		Walnut::Timer timer;

		m_camera.OnResize(m_viewportWidth, m_viewportHeight);

		m_renderer.OnResize(m_viewportWidth, m_viewportHeight);
		m_renderer.Render(m_scene, m_camera);

		m_lastFrameTime = timer.ElapsedMillis();
	}

private:
	float m_lastFrameTime = 0.0f;

	uint32_t m_viewportWidth = 0;
	uint32_t m_viewportHeight = 0;

	Camera m_camera;
	Renderer m_renderer;
	Scene m_scene;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});

	return app;
}
