#include "GaussianBlurWindow.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
sf::Image LoadImage(const std::filesystem::path& path)
{
	if (!std::filesystem::is_regular_file(path))
	{
		throw std::runtime_error("Input image does not exist: " + path.string());
	}

	sf::Image image;
	if (!image.loadFromFile(path))
	{
		throw std::runtime_error("Failed to load image: " + path.string());
	}
	return image;
}

sf::Vector2u GetWindowSize(const sf::Vector2u imageSize)
{
	return {
		std::clamp(imageSize.x, 640U, 1280U),
		std::clamp(imageSize.y, 360U, 800U) + 72U
	};
}
} // namespace

GaussianBlurWindow::GaussianBlurWindow(
	const std::filesystem::path& inputPath,
	const std::filesystem::path& outputPath)
	: m_outputPath(outputPath)
	, m_sourceImage(LoadImage(inputPath))
	, m_window(sf::VideoMode(GetWindowSize(m_sourceImage.getSize())), "Gaussian Blur")
	, m_blur(m_sourceImage)
	, m_resultTexture(m_sourceImage.getSize())
{
	if (!m_window.isOpen())
	{
		throw std::runtime_error("Failed to create the application window");
	}

	m_window.setVerticalSyncEnabled(true);
	m_resultTexture.setSmooth(false);
	SetRadius(m_radius);
}

void GaussianBlurWindow::Run()
{
	while (m_window.isOpen())
	{
		while (const std::optional event = m_window.pollEvent())
		{
			HandleEvent(*event);
		}
		Draw();
	}
}

void GaussianBlurWindow::HandleEvent(const sf::Event& event)
{
	if (event.is<sf::Event::Closed>())
	{
		m_window.close();
	}
	else if (const auto* key = event.getIf<sf::Event::KeyPressed>())
	{
		if (key->code == sf::Keyboard::Key::Escape)
		{
			m_window.close();
		}
		else if (key->code == sf::Keyboard::Key::S)
		{
			SaveResult();
		}
	}
	else if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>())
	{
		const sf::Vector2f position(
			static_cast<float>(press->position.x),
			static_cast<float>(press->position.y));
		if (press->button == sf::Mouse::Button::Left
			&& GetSliderBounds().contains(position))
		{
			m_dragging = true;
			SetRadius(GetRadiusFromMouse(press->position.x));
		}
	}
	else if (const auto* release = event.getIf<sf::Event::MouseButtonReleased>())
	{
		if (release->button == sf::Mouse::Button::Left)
		{
			m_dragging = false;
		}
	}
	else if (const auto* move = event.getIf<sf::Event::MouseMoved>())
	{
		if (m_dragging)
		{
			SetRadius(GetRadiusFromMouse(move->position.x));
		}
	}
	else if (const auto* resized = event.getIf<sf::Event::Resized>())
	{
		m_window.setView(sf::View(sf::FloatRect(
			{ 0.0f, 0.0f },
			{ static_cast<float>(resized->size.x), static_cast<float>(resized->size.y) })));
	}
}

void GaussianBlurWindow::SetRadius(const int radius)
{
	if (radius == m_radius && m_hasResult)
	{
		return;
	}
	m_radius = radius;
	m_blur.Apply(m_radius);
	m_resultTexture.update(m_blur.GetPixels().data());
	m_hasResult = true;
	UpdateTitle();
}

void GaussianBlurWindow::SaveResult() const
{
	if (const sf::Image result(m_blur.GetSize(), m_blur.GetPixels().data());
		!result.saveToFile(m_outputPath))
	{
		throw std::runtime_error("Failed to save image: " + m_outputPath.string());
	}
	std::cout << "Saved: " << m_outputPath << '\n';
}

void GaussianBlurWindow::UpdateTitle()
{
	std::ostringstream title;
	title << "Gaussian Blur | radius: " << m_radius;
	m_window.setTitle(title.str());
}

void GaussianBlurWindow::Draw()
{
	m_window.clear(sf::Color(24, 27, 32));
	DrawImage();
	DrawSlider();
	m_window.display();
}

void GaussianBlurWindow::DrawImage()
{
	const sf::Vector2u windowSize = m_window.getSize();
	const sf::Vector2u imageSize = m_resultTexture.getSize();
	const float width = static_cast<float>(windowSize.x);
	const float height = std::max(
		1.0f,
		static_cast<float>(windowSize.y) - static_cast<float>(CONTROL_PANEL_HEIGHT));
	const float scale = std::min(
		width / static_cast<float>(imageSize.x),
		height / static_cast<float>(imageSize.y));

	sf::Sprite sprite(m_resultTexture);
	sprite.setScale({ scale, scale });
	sprite.setPosition({ (width - static_cast<float>(imageSize.x) * scale) * 0.5f,
		(height - static_cast<float>(imageSize.y) * scale) * 0.5f });
	m_window.draw(sprite);
}

void GaussianBlurWindow::DrawSlider()
{
	const sf::FloatRect bounds = GetSliderBounds();
	const float centerY = bounds.position.y + bounds.size.y * 0.5f;
	const float fraction = static_cast<float>(m_radius)
		/ static_cast<float>(GpuGaussianBlur::MAX_RADIUS);

	sf::RectangleShape track({ bounds.size.x, SLIDER_HEIGHT });
	track.setPosition({ bounds.position.x, centerY - SLIDER_HEIGHT * 0.5f });
	track.setFillColor(sf::Color(70, 74, 82));
	m_window.draw(track);

	sf::RectangleShape progress({ bounds.size.x * fraction, SLIDER_HEIGHT });
	progress.setPosition({ bounds.position.x, centerY - SLIDER_HEIGHT * 0.5f });
	progress.setFillColor(sf::Color(65, 145, 235));
	m_window.draw(progress);

	sf::CircleShape knob(KNOB_RADIUS);
	knob.setOrigin({ KNOB_RADIUS, KNOB_RADIUS });
	knob.setPosition({ bounds.position.x + bounds.size.x * fraction, centerY });
	knob.setFillColor(sf::Color::White);
	knob.setOutlineColor(sf::Color(30, 33, 38));
	knob.setOutlineThickness(2.0f);
	m_window.draw(knob);
}

sf::FloatRect GaussianBlurWindow::GetSliderBounds() const
{
	const sf::Vector2u size = m_window.getSize();
	const float y = static_cast<float>(size.y) - 38.0f;
	return {
		{ SLIDER_MARGIN, y - KNOB_RADIUS },
		{ std::max(1.0f, static_cast<float>(size.x) - 2.0f * SLIDER_MARGIN),
			2.0f * KNOB_RADIUS }
	};
}

int GaussianBlurWindow::GetRadiusFromMouse(const int mouseX) const
{
	const sf::FloatRect bounds = GetSliderBounds();
	const float position = std::clamp(
		(static_cast<float>(mouseX) - bounds.position.x) / bounds.size.x,
		0.0f,
		1.0f);
	return static_cast<int>(std::lround(position * GpuGaussianBlur::MAX_RADIUS));
}