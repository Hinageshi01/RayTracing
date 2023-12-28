#include "Camera.h"
#include "Renderer.h"

#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Image.h>
#include <Walnut/Timer.h>

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer() : m_camera(45.0f, 0.01f, 1000.0f) {}

	virtual void OnUpdate(float ts) override
	{
		m_camera.OnUpdate(ts);
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Last Frame: %.3fms", m_lastFrameTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}
		ImGui::End();

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

		Render();
	}

	void Render()
	{

		Walnut::Timer timer;

		m_camera.OnResize(m_viewportWidth, m_viewportHeight);

		m_renderer.OnResize(m_viewportWidth, m_viewportHeight);
		m_renderer.Render(m_camera);

		m_lastFrameTime = timer.ElapsedMillis();
	}

private:
	float m_lastFrameTime = 0.0f;

	uint32_t m_viewportWidth = 0;
	uint32_t m_viewportHeight = 0;

	Camera m_camera;
	Renderer m_renderer;
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
