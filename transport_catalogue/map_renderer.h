#pragma once

#include "svg.h"
#include "geo.h"

#include <optional>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string_view>

namespace renderer {

inline const double EPS = 1e-6;

class Projector {
public:
	template<typename PointInputIt>
	Projector(PointInputIt begin, PointInputIt end, double width, double height, double padding)
		: padding_(padding) {
		using namespace std;
		const auto [left_it, right_it] = minmax_element(begin, end, [](const auto& lhs, const auto& rhs) {
			return lhs.lng < rhs.lng; });
		min_lon_ = left_it->lng;
		const double max_lon = right_it->lng;

		const auto [bottom_it, top_it] = minmax_element(begin, end, [](const auto& lhs, const auto& rhs) {
			return lhs.lat < rhs.lat; });
		max_lat_ = top_it->lat;
		const double min_lat = bottom_it->lat;

		if (IsZero(max_lon - min_lon_) && !IsZero(max_lat_ - min_lat)) {
			zoom_coef_ = (height - 2 * padding_) / (max_lat_ - min_lat);
		}
		else if (!IsZero(max_lon - min_lon_) && IsZero(max_lat_ - min_lat)) {
			zoom_coef_ = (width - 2 * padding_) / (max_lon - min_lon_);
		}
		else if (IsZero(max_lon - min_lon_) && IsZero(max_lat_ - min_lat)) {
			zoom_coef_ = 0;
		}
		else if (!IsZero(max_lon - min_lon_) && !IsZero(max_lat_ - min_lat)) {
			double width_zoom_coef = (width - 2 * padding_) / (max_lon - min_lon_);
			double height_zoom_coef = (height - 2 * padding_) / (max_lat_ - min_lat);
			zoom_coef_ = min(width_zoom_coef, height_zoom_coef);
		}
	}

	svg::Point operator()(geo::Coordinates coord) const {
		return { (coord.lng - min_lon_) * zoom_coef_ + padding_
			, (max_lat_ - coord.lat) * zoom_coef_ + padding_ };
	}
private:
	double min_lon_ = 0.0;
	double max_lat_ = 0.0;
	double zoom_coef_ = 0.0;
	double padding_ = 0.0;

	bool IsZero(double value) {
		return std::abs(value) < EPS;
	}
};

struct Settings {
	double width = 0.0;
	double height = 0.0;
	double padding = 0.0;
	double line_width = 0.0;
	double stop_radius = 0.0;
	int bus_label_font_size = 0;
	svg::Point bus_label_offset;
	int stop_label_font_size = 0;
	svg::Point stop_label_offset;
	svg::Color underlayer_color;
	double underlayer_width = 0.0;
	std::vector<svg::Color> color_palette;
};

class MapRenderer{
public:
	MapRenderer(Settings settings);

	svg::Polyline RenderPolyline(const std::vector<svg::Point>&, int) const;

	svg::Text RenderTextSubstrate(svg::Point, std::string_view) const;

	svg::Text RenderText(svg::Point, std::string_view, int) const;

	svg::Circle RenderStopCircle(svg::Point) const;

	svg::Text RenderStopTextSubstrate(svg::Point, std::string_view) const;

	svg::Text RenderStopText(svg::Point, std::string_view) const;

	double GetWidth() const;
	double GetHeight() const;
	double GetPadding() const;

	const Settings& GetSettings() const;
private:
	Settings settings_;

	void SetTextProps(svg::Text&, svg::Point, std::string_view) const;

	void SetStopTextProps(svg::Text&, svg::Point, std::string_view) const;
};

}