#include "request_handler.h"

#include <set>
#include <string_view>
#include "json_builder.h"

RequestHandler::RequestHandler(const catalogue::TransportCatalogue& db, const renderer::MapRenderer& renderer, const catalogue::transport_router::TransportRouter& router) 
	: db_(db)
	, renderer_(renderer)
	, router_(router) {
}

const catalogue::BusInfo* RequestHandler::GetBusStat(std::string_view bus_name) const {
	return db_.GetBusInfo(bus_name);
}

std::optional<catalogue::StopInfo> RequestHandler::GetBusesByStop(std::string_view stop_name) const
{
	return db_.GetStopInfo(stop_name);
}

svg::Document RequestHandler::RenderMap() const
{
	svg::Document map;
	//buses to out
	std::set<std::string_view> buses_names;
	for (const auto& bus : db_.GetBuses()) {
		if (bus.stops.size() != 0) {
			buses_names.insert(bus.name);
		}
	}
	//stop_points to compute projector and stop_names to render circles
	std::vector<geo::Coordinates> stop_coord;
	std::set<std::string_view> stop_names;
	for (const auto& stop : db_.GetStops()) {
		if (db_.GetStopInfo(stop.name) != nullptr) {
			stop_coord.push_back(stop.coordinates);
			stop_names.insert(stop.name);
		}
	}
	renderer::Projector projector(stop_coord.begin(), stop_coord.end(), renderer_.GetWidth(), renderer_.GetHeight(), renderer_.GetPadding());
	//out bus lines
	int bus_number = 0;
	for (std::string_view bus_name : buses_names) {
		std::vector<svg::Point> points;
		for (const catalogue::Stop* stop : db_.GetBusByName(bus_name)->stops) {
			points.push_back(projector(stop->coordinates));
		}
		map.Add(renderer_.RenderPolyline(points, bus_number));
		bus_number++;
	}
	//out ending stations names
	bus_number = 0;
	for (std::string_view bus_name : buses_names) {
		const auto& bus = db_.GetBusByName(bus_name);
		if (bus->is_roundtrip) {
			const catalogue::Stop* ending = bus->stops.front();
			map.Add(renderer_.RenderTextSubstrate(projector(ending->coordinates), bus_name));
			map.Add(renderer_.RenderText(projector(ending->coordinates), bus_name, bus_number));
		}
		else {
			const catalogue::Stop* first_ending = bus->stops.front();
			auto it = bus->stops.begin();
			int steps = bus->stops.size() / 2;
			std::advance(it, steps);
			const catalogue::Stop* second_ending = *it;
			map.Add(renderer_.RenderTextSubstrate(projector(first_ending->coordinates), bus_name));
			map.Add(renderer_.RenderText(projector(first_ending->coordinates), bus_name, bus_number));
			if (first_ending != second_ending) {
				map.Add(renderer_.RenderTextSubstrate(projector(second_ending->coordinates), bus_name));
				map.Add(renderer_.RenderText(projector(second_ending->coordinates), bus_name, bus_number));
			}
		}
		bus_number++;
	}
	//out stop's round
	for (std::string_view stop_name : stop_names) {
		svg::Point pos = projector(db_.GetStopByName(stop_name)->coordinates);
		map.Add(renderer_.RenderStopCircle(pos));
	}
	//out stop's names
	for (std::string_view stop_name : stop_names) {
		svg::Point pos = projector(db_.GetStopByName(stop_name)->coordinates);
		map.Add(renderer_.RenderStopTextSubstrate(pos, stop_name));
		map.Add(renderer_.RenderStopText(pos, stop_name));
	}
	return map;
}

json::Node RequestHandler::Route(std::string_view from, std::string_view to) const
{
	using namespace std::string_literals;
	if (from == to) {
		return json::Builder{}.StartDict()
			.Key("total_time"s).Value(0.0)
			.Key("items"s).StartArray().EndArray()
			.EndDict().Build();
	}
	std::optional<catalogue::transport_router::BuiltRoute> built_route = router_.Route(from, to);
	if (!built_route) {
		return json::Node();
	}
	json::Array out;

	for (size_t i = 0; i < built_route->wait_items.size(); ++i) {
		out.push_back(json::Builder{}.StartDict()
			.Key("type"s).Value("Wait"s)
			.Key("stop_name").Value(built_route->wait_items[i].name)
			.Key("time"s).Value(built_route->wait_items[i].time)
			.EndDict().Build().AsDict());
		out.push_back(json::Builder{}.StartDict()
			.Key("type"s).Value("Bus"s)
			.Key("bus"s).Value(built_route->bus_items[i].name)
			.Key("span_count"s).Value(built_route->bus_items[i].span_count)
			.Key("time"s).Value(built_route->bus_items[i].time)
			.EndDict().Build().AsDict());
	}
	return json::Builder{}.StartDict()
		.Key("total_time"s).Value(built_route->total_time)
		.Key("items"s).Value(out)
		.EndDict().Build();
}
