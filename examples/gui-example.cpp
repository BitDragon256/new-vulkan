#include <render.h>
#include <imgui.h>

int main()
{
	Renderer renderer;
	RenderConfig config;
	config.width = 800;
	config.height = 600;
	config.title = "GUI Example";
	renderer.init(config);

	while (true)
	{
		renderer.gui_begin();

		ImGui::Begin("Window 1");

		float colors[4];
		ImGui::ColorPicker4("Color picker", colors);

		ImGui::End();

		ImGui::Begin("Window 2");

		float floats[3];
		ImGui::DragFloat3("Value Sliders", floats, 0.3f, -10, 10);

		ImGui::End();

		auto res = renderer.render();
		if (res == NVE_RENDER_EXIT_SUCCESS)
			break;
	}
}