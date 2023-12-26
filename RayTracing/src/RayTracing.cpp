#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

class ExampleLayer : public Walnut::Layer
{
public:
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

		if (m_pImage)
		{
			ImGui::Image(m_pImage->GetDescriptorSet(), ImVec2{ (float)m_pImage->GetWidth(), (float)m_pImage->GetHeight() });
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void Render()
	{

		Walnut::Timer timer;

		if (!m_pImage || m_pImage->GetWidth() != m_viewportWidth || m_pImage->GetHeight() != m_viewportHeight)
		{
			m_pImage.reset(new Walnut::Image(m_viewportWidth, m_viewportHeight, Walnut::ImageFormat::RGBA));

			delete[] m_pImageData;
			m_pImageData = new uint32_t[m_viewportWidth * m_viewportHeight];
		}

		for (size_t i = 0; i < m_viewportWidth * m_viewportHeight; ++i)
		{
			m_pImageData[i] = Walnut::Random::UInt();
			m_pImageData[i] |= 0xFF00000000;
		}
		m_pImage->SetData(m_pImageData);

		m_lastFrameTime = timer.ElapsedMillis();
	}

private:
	float m_lastFrameTime = 0.0f;

	uint32_t m_viewportWidth = 0;
	uint32_t m_viewportHeight = 0;

	uint32_t *m_pImageData = nullptr;
	std::shared_ptr<Walnut::Image> m_pImage;
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