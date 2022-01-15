#include "transport_router.h"

#include <unordered_set>
#include <optional>
#include <utility>
#include <string>
#include <list>

namespace catalogue {
	namespace transport_router {

		TransportRouter::TransportRouter(Graph graph, StopVertexId svi, VertexIdStop vis, EdgesExtraInfo eei, double bus_wait_time)
			: graph_(std::move(graph))
			, router_(graph_)
			, stop_vertexid_(move(svi))
			, vertexid_stop_(move(vis))
			, edges_extra_info_(move(eei))
			, bus_wait_time_(bus_wait_time) {
		}

		TransportRouter::TransportRouter(Graph graph, graph::Router<double>::RoutesInternalData router_internal_data, StopVertexId svi, VertexIdStop vis, EdgesExtraInfo eei, double bus_wait_time)
			: graph_(std::move(graph))
			, router_(graph_, std::move(router_internal_data))
			, stop_vertexid_(std::move(svi))
			, vertexid_stop_(std::move(vis))
			, edges_extra_info_(std::move(eei))
			, bus_wait_time_(bus_wait_time) {
		}

		std::optional<BuiltRoute> TransportRouter::Route(std::string_view from, std::string_view to) const
		{
			if (stop_vertexid_.count(from) == 0 || stop_vertexid_.count(to) == 0) {
				return std::nullopt;
			}
			auto route_info = router_.BuildRoute(stop_vertexid_.at(from), stop_vertexid_.at(to));
			if (!route_info.has_value()) {
				return std::nullopt;
			}

			BuiltRoute built_route;
			built_route.total_time = route_info->weight;
			BuiltRoute::RouteWaitInfo route_wait_info;
			BuiltRoute::RouteBusInfo route_bus_info;
			for (graph::EdgeId edge_id : route_info.value().edges) {
				//WAIT
				route_wait_info.name = static_cast<std::string>(vertexid_stop_.at(graph_.GetEdge(edge_id).from));
				route_wait_info.time = bus_wait_time_;
				built_route.wait_items.push_back(route_wait_info);
				//BUS
				route_bus_info.name = static_cast<std::string>(edges_extra_info_.at(edge_id).bus_name);
				route_bus_info.time = graph_.GetEdge(edge_id).weight - bus_wait_time_;
				route_bus_info.span_count = edges_extra_info_.at(edge_id).span_count;
				built_route.bus_items.push_back(route_bus_info);
			}

			return built_route;
		}

		TransportRouter::TransportRouterData TransportRouter::GetTransportRouterData() const
		{
			return { graph_, router_, stop_vertexid_, edges_extra_info_, bus_wait_time_ };
		}

		TransportRouter MakeTransportRouter(const TransportCatalogue& catalogue, double bus_wait_time, double bus_velocity) {
			TransportRouter::StopVertexId stop_vertexid;
			TransportRouter::VertexIdStop vertexid_stop;
			TransportRouter::EdgesExtraInfo edges_extra_info;
			std::vector<graph::Edge<double>> edges;

			const Stop* last_stop = nullptr;
			int span_count = 0;
			double distance = 0.0;
			graph::VertexId current_vertex_id = 0,
				from = 0,
				to = 0;
			for (const auto& stop : catalogue.GetStops()) {
				stop_vertexid[stop.name] = current_vertex_id;
				vertexid_stop[current_vertex_id++] = stop.name;
			}
			for (const Bus& bus : catalogue.GetBuses()) {
				auto final_station = bus.stops.begin();
				if (!bus.is_roundtrip) {
					int steps = bus.stops.size() / 2;
					std::advance(final_station, steps);
				}
				for (auto i = bus.stops.begin(); i != bus.stops.end(); i++) {
					#define from_stop (*i)
					span_count = 0;
					distance = 0.0;
					last_stop = from_stop;
					for (auto j = std::next(i, 1); j != bus.stops.end(); j++) {
						#define to_stop (*j)
						if (from_stop == to_stop) {
							last_stop = to_stop;
							continue;
						}
						distance += catalogue.GetDistance(last_stop->name, to_stop->name);
						span_count++;
						from = stop_vertexid.at(from_stop->name);
						to = stop_vertexid.at(to_stop->name);
						edges.push_back({ from, to, distance / bus_velocity + bus_wait_time });
						edges_extra_info.push_back({ bus.name, span_count });
						last_stop = to_stop;
						if (j == final_station) {
							break;
						}
					}
				}
			}
			TransportRouter::Graph graph(current_vertex_id);
			for (graph::Edge<double>& edge : edges) {
				graph.AddEdge(std::move(edge));
			}
			return TransportRouter{ graph, stop_vertexid, vertexid_stop, edges_extra_info, bus_wait_time };
		}

	}
}