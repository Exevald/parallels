#pragma once

#include "GpuGaussianBlur.h"

#include <SFML/Graphics.hpp>
#include <filesystem>

class GaussianBlurWindow
{
public:
	GaussianBlurWindow(
		const std::filesystem::path& inputPath,
		const std::filesystem::path& outputPath);

	void Run();

private:
	static constexpr unsigned int CONTROL_PANEL_HEIGHT = 72;
	static constexpr float SLIDER_MARGIN = 36.0f;
	static constexpr float SLIDER_HEIGHT = 8.0f;
	static constexpr float KNOB_RADIUS = 11.0f;

	void HandleEvent(const sf::Event& event);
	void SetRadius(int radius);
	void SaveResult() const;
	void UpdateTitle();
	void Draw();
	void DrawImage();
	void DrawSlider();

	[[nodiscard]] sf::FloatRect GetSliderBounds() const;
	[[nodiscard]] int GetRadiusFromMouse(int mouseX) const;

	std::filesystem::path m_outputPath;
	sf::Image m_sourceImage;
	sf::RenderWindow m_window;
	GpuGaussianBlur m_blur;
	sf::Texture m_resultTexture;
	int m_radius = 10;
	bool m_dragging = false;
	bool m_hasResult = false;
};
