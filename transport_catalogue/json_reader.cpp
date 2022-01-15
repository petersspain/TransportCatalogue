#include "json_reader.h"

#include <cstdint>
#include <sstream>
#include <filesystem>

#include "json_builder.h"

namespace reader {

using namespace json;
using namespace std::string_literals;

struct toStopDistance {
	std::string_view from;
	std::string_view to;
	uint32_t distance;
};

catalogue::TransportCatalogue ParseBaseRequests(const Node& base_req) {
	catalogue::TransportCatalogue tc;
	std::vector<const Node*> buses_req;
	std::vector<toStopDistance> distances_;
	//adding stops and saving distances, buses for further using
	for (const Node& node : base_req.AsArray()) {
		const Dict& stop_dict = node.AsDict();
		const auto& type = stop_dict.at("type"s);
		if (type == "Stop"s) {
			catalogue::Stop stop;
			stop.name = stop_dict.at("name"s).AsString();
			stop.coordinates = { stop_dict.at("latitude"s).AsDouble(), stop_dict.at("longitude"s).AsDouble() };
			tc.AddStop(stop);
			if (stop_dict.count("road_distances"s) != 0) {
				for (const auto& [key, value] : stop_dict.at("road_distances"s).AsDict()) {
					distances_.push_back({ stop_dict.at("name"s).AsString(), key, static_cast<uint32_t>(value.AsInt()) });
				}
			}
		}
		else if (type == "Bus"s) {
			buses_req.push_back(&node);
		}
	}
	//adding distances
	for (const toStopDistance& tsd : distances_) {
		tc.AddDistance(tsd.from, tsd.to, tsd.distance);
	}
	//adding buses
	for (const Node* bus_req : buses_req) {
		const Dict& bus_dict = bus_req->AsDict();
		catalogue::Bus bus;
		bus.name = bus_dict.at("name"s).AsString();
		bus.is_roundtrip = true;
		for (const auto& stop : bus_dict.at("stops"s).AsArray()) {
			//pushing const Stop* pointers to bus
			bus.stops.push_back(tc.GetStopByName(stop.AsString()));
		}
		if (!bus_dict.at("is_roundtrip"s).AsBool()) {
			//bus isnt roundtrip -> we should add reverse bus way to stops
			bus.is_roundtrip = false;
			std::vector<const catalogue::Stop*> reverse_stops(std::next(bus.stops.rbegin(), 1), bus.stops.rend());
			for (auto it = reverse_stops.begin(); it != reverse_stops.end(); std::advance(it, 1)) {
				bus.stops.push_back(*it);
			}
		}
		tc.AddBus(bus);
	}
	return tc;
}

svg::Color GetColor(const Node& color_node) {
	svg::Color color;
	if (color_node.IsString()) {
		color = color_node.AsString();
	}
	else if (color_node.IsArray()) {
		const Array& color_arr = color_node.AsArray();
		if (color_arr.size() == 3) {
			color = svg::Rgb{ static_cast<uint8_t>(color_arr[0].AsInt()), 
				static_cast<uint8_t>(color_arr[1].AsInt()), 
				static_cast<uint8_t>(color_arr[2].AsInt()) };
		}
		else if (color_arr.size() == 4) {
			color = svg::Rgba{ static_cast<uint8_t>(color_arr[0].AsInt()),
				static_cast<uint8_t>(color_arr[1].AsInt()),
				static_cast<uint8_t>(color_arr[2].AsInt()),
				color_arr[3].AsDouble() };
		}
	}
	return color;
}

renderer::MapRenderer ParseRenderRequests(const json::Node& render_sett) {
	renderer::Settings settings;
	const Dict& dict = render_sett.AsDict();
	settings.width = dict.at("width"s).AsDouble();
	settings.height = dict.at("height"s).AsDouble();
	settings.padding = dict.at("padding"s).AsDouble();
	settings.line_width = dict.at("line_width"s).AsDouble();
	settings.stop_radius = dict.at("stop_radius").AsDouble();
	settings.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();
	const Array& bus_label_offset = dict.at("bus_label_offset"s).AsArray();
	settings.bus_label_offset = { bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble() };
	settings.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();
	const Array& stop_label_offset = dict.at("stop_label_offset"s).AsArray();
	settings.stop_label_offset = { stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble() };
	settings.underlayer_color = GetColor(dict.at("underlayer_color"s));
	settings.underlayer_width = dict.at("underlayer_width"s).AsDouble();
	const Array& color_palette = dict.at("color_palette"s).AsArray();
	for (const Node& color_node : color_palette) {
		settings.color_palette.push_back(GetColor(color_node));
	}
	return renderer::MapRenderer(settings);
}

Document ParseStatRequests(const RequestHandler& rh, const Node& stat_req) {
	Array out;
	for (const Node& node : stat_req.AsArray()) {
		const Dict& stop_dict = node.AsDict();
		Dict dict;
		const auto& type = stop_dict.at("type"s);
		if (type == "Stop"s) {
			const std::string& name = stop_dict.at("name"s).AsString();
			if (auto stops_set = rh.GetBusesByStop(name)) {
				Array stops_arr;
				if (*stops_set != nullptr) {
					for (const std::string_view stop : **stops_set) {
						stops_arr.push_back(std::string(stop));
					}
				}
				dict = json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
						.Key("buses"s).Value(stops_arr)
					.EndDict()
					.Build().AsDict();
			}
			else {
				dict = json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
						.Key("error_message"s).Value("not found"s)
					.EndDict()
					.Build().AsDict();
			}
		}
		else if (type == "Bus"s) {
			const std::string& name = stop_dict.at("name"s).AsString();
			if (auto bus_info = rh.GetBusStat(name)) {
				dict = json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
						.Key("curvature"s).Value((*bus_info).curvature)
						.Key("route_length"s).Value((*bus_info).route_length)
						.Key("stop_count"s).Value((*bus_info).stops)
						.Key("unique_stop_count"s).Value((*bus_info).unique_stops)
					.EndDict()
					.Build().AsDict();
			}
			else {
				dict = json::Builder{}
					.StartDict()
						.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
						.Key("error_message"s).Value("not found"s)
					.EndDict()
					.Build().AsDict();
			}
		}
		else if (type == "Map"s) {
			std::ostringstream out_stream;
			rh.RenderMap().Render(out_stream);
			dict = json::Builder{}
				.StartDict()
					.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
					.Key("map"s).Value(out_stream.str())
				.EndDict()
				.Build().AsDict();
		}
		else if (type == "Route"s) {
			const Node& route_info = rh.Route(stop_dict.at("from"s).AsString(), stop_dict.at("to"s).AsString());
			if (route_info.IsNull()) {
				dict = json::Builder{}
					.StartDict()
					.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
					.Key("error_message"s).Value("not found"s)
					.EndDict().Build().AsDict();
			}
			else {
				const Dict& route_dict = route_info.AsDict();
				dict = json::Builder{}
					.StartDict()
					.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
					.Key("total_time"s).Value(route_dict.at("total_time"s).AsDouble())
					.Key("items"s).Value(route_dict.at("items"s).AsArray())
					.EndDict().Build().AsDict();
			}
		}
		else {
			dict = json::Builder{}
				.StartDict()
					.Key("request_id"s).Value(stop_dict.at("id"s).AsInt())
				.EndDict()
				.Build().AsDict();
		}
		out.push_back(dict);
	}
	return Document{ out };
}

catalogue::transport_router::TransportRouter ParseRoutingSettingsRequest(const catalogue::TransportCatalogue& tc, const json::Node& routing_settings) {
	const Dict& settings = routing_settings.AsDict();
	double bus_velocity_at_meters_min = settings.at("bus_velocity"s).AsDouble() * 1000.0 / 60.0;
	return catalogue::transport_router::MakeTransportRouter(tc, settings.at("bus_wait_time"s).AsDouble(), bus_velocity_at_meters_min);
}

catalogue::Serialization ParseSerializationSettings(const json::Node& ser_sett) {
	return catalogue::Serialization(ser_sett.AsDict().at("file"s).AsString());
}

}