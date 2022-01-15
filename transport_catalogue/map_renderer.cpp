#include "map_renderer.h"

#include <string>

namespace renderer {
	MapRenderer::MapRenderer(Settings settings) 
		: settings_(settings) {
	}

	svg::Polyline MapRenderer::RenderPolyline(const std::vector<svg::Point>& coords, int bus_number) const {
		svg::Polyline pl;
		pl.SetFillColor(svg::NoneColor);
		pl.SetStrokeColor(settings_.color_palette[bus_number % settings_.color_palette.size()]);
		pl.SetStrokeWidth(settings_.line_width);
		pl.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		pl.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		for (const auto& coord : coords) {
			pl.AddPoint(coord);
		}
		return pl;
	}

	svg::Circle MapRenderer::RenderStopCircle(svg::Point pos) const
	{
		svg::Circle circle;
		circle.SetCenter(pos);
		circle.SetRadius(settings_.stop_radius);
		circle.SetFillColor("white");
		return circle;
	}

	svg::Text MapRenderer::RenderTextSubstrate(svg::Point pos, std::string_view name) const {
		svg::Text text;
		SetTextProps(text, pos, name);
		text.SetFillColor(settings_.underlayer_color);
		text.SetStrokeColor(settings_.underlayer_color);
		text.SetStrokeWidth(settings_.underlayer_width);
		text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		return text;
	}

	svg::Text MapRenderer::RenderText(svg::Point pos, std::string_view name, int bus_number) const
	{
		svg::Text text;
		SetTextProps(text, pos, name);
		text.SetFillColor(settings_.color_palette[bus_number % settings_.color_palette.size()]);
		return text;
	}

	void MapRenderer::SetTextProps(svg::Text& text, svg::Point point, std::string_view name) const {
		text.SetPosition(point);
		text.SetOffset(settings_.bus_label_offset);
		text.SetFontSize(settings_.bus_label_font_size);
		text.SetFontFamily("Verdana");
		text.SetFontWeight("bold");
		text.SetData(std::string(name));
	}

	svg::Text MapRenderer::RenderStopText(svg::Point pos, std::string_view data) const {
		svg::Text text;
		SetStopTextProps(text, pos, data);
		text.SetFillColor("black");
		return text;
	}

	svg::Text MapRenderer::RenderStopTextSubstrate(svg::Point pos, std::string_view data) const	{
		svg::Text text;
		SetStopTextProps(text, pos, data);
		text.SetFillColor(settings_.underlayer_color);
		text.SetStrokeColor(settings_.underlayer_color);
		text.SetStrokeWidth(settings_.underlayer_width);
		text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		return text;
	}

	void MapRenderer::SetStopTextProps(svg::Text& text, svg::Point pos, std::string_view data) const {
		text.SetPosition(pos);
		text.SetOffset(settings_.stop_label_offset);
		text.SetFontSize(settings_.stop_label_font_size);
		text.SetFontFamily("Verdana");
		text.SetData(std::string(data));
	}

	double MapRenderer::GetWidth() const
	{
		return settings_.width;
	}

	double MapRenderer::GetHeight() const
	{
		return settings_.height;
	}

	double MapRenderer::GetPadding() const
	{
		return settings_.padding;
	}

	const Settings& MapRenderer::GetSettings() const {
		return settings_;
	}
}