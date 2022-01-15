#include "serialization.h"
#include "transport_catalogue.pb.h"
#include "svg.h"

#include <fstream>
#include <string_view>
#include <string>
#include <variant>
#include <vector>
#include <map>

namespace catalogue {

	transport_catalogue::Color GetMessageColor(const svg::Color& color) {
		transport_catalogue::Color color_message;
		if (std::holds_alternative<std::string>(color)) {
			color_message.set_str_color(std::get<std::string>(color));
		}
		else if (std::holds_alternative<svg::Rgb>(color)) {
			color_message.mutable_rgb_color()->set_r(std::get<svg::Rgb>(color).red);
			color_message.mutable_rgb_color()->set_g(std::get<svg::Rgb>(color).green);
			color_message.mutable_rgb_color()->set_b(std::get<svg::Rgb>(color).blue);
		}
		else if (std::holds_alternative<svg::Rgba>(color)) {
			color_message.mutable_rgba_color()->mutable_rgb()->set_r(std::get<svg::Rgba>(color).red);
			color_message.mutable_rgba_color()->mutable_rgb()->set_g(std::get<svg::Rgba>(color).green);
			color_message.mutable_rgba_color()->mutable_rgb()->set_b(std::get<svg::Rgba>(color).blue);
			color_message.mutable_rgba_color()->set_a(std::get<svg::Rgba>(color).opacity);
		}
		return color_message;
	}

	void Serialization::SerializeCatalogue(const catalogue::TransportCatalogue& database, const renderer::MapRenderer& map_renderer, const catalogue::transport_router::TransportRouter& transport_router) const
	{
		transport_catalogue::Data data_to_store;
		transport_catalogue::TransportCatalogue& catalogue_message = *data_to_store.mutable_transport_catalogue();
		//заполняем Stop
		{
			transport_catalogue::Stop stop_message;
			for (const catalogue::Stop& stop : database.GetStops()) {
				stop_message.set_name(stop.name);
				(*stop_message.mutable_coordinates()).set_lat(stop.coordinates.lat);
				(*stop_message.mutable_coordinates()).set_lng(stop.coordinates.lng);
				*catalogue_message.add_stops() = stop_message;
			}
		}
		//Заполняем Distance
		{
			transport_catalogue::Distance distance_message;
			for (const auto& [from_to, distance] : database.GetStopsDistances()) {
				distance_message.set_from(from_to.first->name);
				distance_message.set_to(from_to.second->name);
				distance_message.set_distance(distance);
				*catalogue_message.add_distances() = distance_message;
			}
		}
		//заполняем buses
		{
			transport_catalogue::Bus message_bus;
			for (const Bus& bus : database.GetBuses()) {
				message_bus.set_name(bus.name);
				message_bus.set_is_roundtrip(bus.is_roundtrip);
				for (const Stop* stop : bus.stops) {
					*message_bus.add_stops() = stop->name;
				}
				*catalogue_message.add_buses() = message_bus;
				message_bus.clear_stops();
			}
		}

		//заполняем Settings
		transport_catalogue::Settings& settings_message = *data_to_store.mutable_settings();
		{
			const renderer::Settings& settings = map_renderer.GetSettings();
			settings_message.set_width(settings.width);
			settings_message.set_height(settings.height);
			settings_message.set_padding(settings.padding);
			settings_message.set_line_width(settings.line_width);
			settings_message.set_stop_radius(settings.stop_radius);
			settings_message.set_bus_label_font_size(settings.bus_label_font_size);
			settings_message.set_stop_label_font_size(settings.stop_label_font_size);
			settings_message.set_underlayer_width(settings.underlayer_width);
			settings_message.mutable_bus_label_offset()->set_x(settings.bus_label_offset.x);
			settings_message.mutable_bus_label_offset()->set_y(settings.bus_label_offset.y);
			settings_message.mutable_stop_label_offset()->set_x(settings.stop_label_offset.x);
			settings_message.mutable_stop_label_offset()->set_y(settings.stop_label_offset.y);

			*settings_message.mutable_underlayer_color() = GetMessageColor(settings.underlayer_color);
			for (const svg::Color& color : settings.color_palette) {
				*settings_message.mutable_color_palette()->Add() = GetMessageColor(color);
			}
		}

		//Заполняем transport_router_data
		transport_catalogue::TransportRouterData& transport_router_data_message = *data_to_store.mutable_transport_router_data();
		const catalogue::transport_router::TransportRouter::TransportRouterData& transport_router_data = transport_router.GetTransportRouterData();
		{
			//bus_wait_time
			transport_router_data_message.set_bus_wait_time(transport_router_data.bus_wait_time);
			//stop_vertex_id
			for (const auto [stop, vertex_id] : transport_router_data.stop_vertex_id) {
				transport_catalogue::StopVertexId& elem = *transport_router_data_message.add_stop_vertex_id();
				elem.set_stop(static_cast<std::string>(stop));
				elem.set_vertex_id(vertex_id);
			}
			//edges_extra_info
			for (const auto [bus_name, span_count] : transport_router_data.edges_extra_info) {
				transport_catalogue::EdgeExtraInfo& elem = *transport_router_data_message.add_edges_extra_info();
				elem.set_bus_name(static_cast<std::string>(bus_name));
				elem.set_span_count(span_count);
			}
			//заполняем поле Graph
			transport_catalogue::Graph& graph = *transport_router_data_message.mutable_graph();
			graph.set_vertex_count(transport_router_data.graph.GetVertexCount());
			for (size_t i = 0; i < transport_router_data.graph.GetEdgeCount(); ++i) {
				const graph::Edge<double>& edge = transport_router_data.graph.GetEdge(i);
				transport_catalogue::Edge& elem = *graph.add_edges();
				elem.set_from(edge.from);
				elem.set_to(edge.to);
				elem.set_weight(edge.weight);
			}
			//заполняем поле Router
			transport_catalogue::Router& router = *transport_router_data_message.mutable_router();
			for (const auto& inner_vector : transport_router_data.router.GetRouterInternalData()) {
				transport_catalogue::VectorRouteInternalData& inner_vector_message = *router.add_vector();
				for (const auto& elem : inner_vector) {
					transport_catalogue::OptionalRouteInternalData& elem_message = *inner_vector_message.add_optional();
					if (elem.has_value()) {
						elem_message.mutable_route_internal_data()->set_weight(elem->weight);
						if (elem->prev_edge.has_value()) {
							elem_message.mutable_route_internal_data()->mutable_prev_edge()->set_id(elem->prev_edge.value());
						}
					}
				}
			}
		}

		std::ofstream out_file(filename_, std::ios::binary);
		data_to_store.SerializeToOstream(&out_file);
	}

	svg::Color GetSvgColor(const transport_catalogue::Color& color_message) {
		svg::Color color;
		if (!color_message.str_color().empty()) {
			color = color_message.str_color();
		}
		else if (color_message.has_rgb_color()) {
			color = svg::Rgb(color_message.rgb_color().r()
				, color_message.rgb_color().g()
				, color_message.rgb_color().b());
		}
		else if (color_message.has_rgba_color()) {
			color = svg::Rgba(color_message.rgba_color().rgb().r()
				, color_message.rgba_color().rgb().g()
				, color_message.rgba_color().rgb().b()
				, color_message.rgba_color().a());
		}
		return color;
	}

	std::optional<Serialization::SerializationOut> Serialization::DeserializeCatalogue() const
	{
		std::ifstream in_file(filename_, std::ios::binary);
		transport_catalogue::Data data_to_parse;
		if (!data_to_parse.ParseFromIstream(&in_file)) {
			return std::nullopt;
		}
		const transport_catalogue::TransportCatalogue& catalogue_message = data_to_parse.transport_catalogue();
		catalogue::TransportCatalogue database;
		//Добавляем Stop
		{
			catalogue::Stop stop;
			for (const transport_catalogue::Stop& stop_message : catalogue_message.stops()) {
				stop.name = stop_message.name();
				stop.coordinates.lat = stop_message.coordinates().lat();
				stop.coordinates.lng = stop_message.coordinates().lng();
				database.AddStop(stop);
			}
		}
		//Добавляем Distance
		{
			for (const transport_catalogue::Distance& distance_message : catalogue_message.distances()) {
				database.AddDistance(distance_message.from(), distance_message.to(), distance_message.distance());
			}
		}
		//Добавляем Bus
		{
			catalogue::Bus bus;
			for (const transport_catalogue::Bus& bus_message : catalogue_message.buses()) {
				bus.name = bus_message.name();
				bus.is_roundtrip = bus_message.is_roundtrip();
				for (const std::string_view& stop_name : bus_message.stops()) {
					bus.stops.push_back(database.GetStopByName(stop_name));
				}
				database.AddBus(bus);
				bus.stops.clear();
			}
		}
		//Добавляем Settings
		const transport_catalogue::Settings& settings_message = data_to_parse.settings();
		renderer::Settings settings;
		{
			settings.width = settings_message.width();
			settings.height = settings_message.height();
			settings.padding = settings_message.padding();
			settings.line_width = settings_message.line_width();
			settings.stop_radius = settings_message.stop_radius();
			settings.bus_label_font_size = settings_message.bus_label_font_size();
			settings.stop_label_font_size = settings_message.stop_label_font_size();
			settings.underlayer_width = settings_message.underlayer_width();
			settings.bus_label_offset.x = settings_message.bus_label_offset().x();
			settings.bus_label_offset.y = settings_message.bus_label_offset().y();
			settings.stop_label_offset.x = settings_message.stop_label_offset().x();
			settings.stop_label_offset.y = settings_message.stop_label_offset().y();
			settings.underlayer_color = GetSvgColor(settings_message.underlayer_color());
			for (const transport_catalogue::Color& color_message : settings_message.color_palette()) {
				settings.color_palette.push_back(GetSvgColor(color_message));
			}
		}
		//Извлекаем transport_router_data
		const transport_catalogue::TransportRouterData& transport_router_data = data_to_parse.transport_router_data();
		//Формируем bus_wait_time
		double bus_wait_time = transport_router_data.bus_wait_time();

		const transport_catalogue::Graph& graph_message = transport_router_data.graph();
		//Формируем graph
		transport_router::TransportRouter::Graph graph(graph_message.vertex_count());
		for (const auto& edge : graph_message.edges()) {
			graph.AddEdge({ edge.from(), edge.to(), edge.weight() });
		}
		//Формируем edges_extra_info
		typename catalogue::transport_router::TransportRouter::EdgesExtraInfo edges_extra_info;
		for (const auto& edge_extra_info_message : transport_router_data.edges_extra_info()) {
			edges_extra_info.push_back({ database.GetBusByName(edge_extra_info_message.bus_name())->name, edge_extra_info_message.span_count() });
		}
		//Формируем StopVertexId и VertexIdStop
		typename catalogue::transport_router::TransportRouter::StopVertexId stop_vertex_id;
		typename catalogue::transport_router::TransportRouter::VertexIdStop vertex_id_stop;
		for (const auto& stop_id : transport_router_data.stop_vertex_id()) {
			std::string_view stop_name = database.GetStopByName(stop_id.stop())->name;
			stop_vertex_id[stop_name] = stop_id.vertex_id();
			vertex_id_stop[stop_id.vertex_id()] = stop_name;
		}
		//Формируем Router
		typename graph::Router<double>::RoutesInternalData routes_internal_data;
		std::vector<std::optional<graph::Router<double>::RouteInternalData>> inner_vector;
		graph::Router<double>::RouteInternalData internal_data;
		for (const auto& inner_vector_message : transport_router_data.router().vector()) {
			for (const auto& optional_data : inner_vector_message.optional()) {
				if (optional_data.has_route_internal_data()) {
					internal_data.weight = optional_data.route_internal_data().weight();
					if (optional_data.route_internal_data().has_prev_edge()) {
						internal_data.prev_edge.emplace(optional_data.route_internal_data().prev_edge().id());
					}
					inner_vector.push_back(internal_data);
					internal_data.prev_edge.reset();
				}
				else {
					inner_vector.push_back(std::nullopt);
				}
			}
			routes_internal_data.push_back(inner_vector);;
			inner_vector.clear();
		}
		return SerializationOut{ std::move(database), settings, {std::move(graph), std::move(routes_internal_data), std::move(stop_vertex_id), std::move(vertex_id_stop), std::move(edges_extra_info), bus_wait_time} };
	}
}