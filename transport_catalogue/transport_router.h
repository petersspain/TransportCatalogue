#pragma once
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <string_view>
#include <map>
#include <vector>
#include <optional>

namespace catalogue {
	namespace transport_router {

		struct BuiltRoute {
			struct RouteWaitInfo {
				std::string name;
				double time = 0.0;
			};
			struct RouteBusInfo : RouteWaitInfo {
				int span_count = 0;
			};
			double total_time = 0.0;
			std::vector<RouteWaitInfo> wait_items;
			std::vector<RouteBusInfo> bus_items;
		};

		struct EdgeExtraInfo {
			std::string_view bus_name;
			int span_count = 0;
		};

		class TransportRouter {
		public:
			using StopVertexId = std::map<std::string_view, graph::VertexId>;
			using VertexIdStop = std::map<graph::VertexId, std::string_view>;
			using EdgesExtraInfo = std::vector<EdgeExtraInfo>;
			using Graph = graph::DirectedWeightedGraph<double>;

			struct TransportRouterData {
				const Graph& graph;
				const graph::Router<double>& router;
				const StopVertexId& stop_vertex_id;
				const EdgesExtraInfo& edges_extra_info;
				const double bus_wait_time = 0.0;
			};

			explicit TransportRouter(Graph, StopVertexId, VertexIdStop, EdgesExtraInfo, double bus_wait_time);//строим граф, а по нему рутер
			explicit TransportRouter(Graph, graph::Router<double>::RoutesInternalData router_internal_data, StopVertexId, VertexIdStop, EdgesExtraInfo, double bus_wait_time);

			std::optional<BuiltRoute> Route(std::string_view from, std::string_view to) const;

			TransportRouterData GetTransportRouterData() const;
		private:
			Graph graph_;
			graph::Router<double> router_;

			StopVertexId stop_vertexid_;
			VertexIdStop vertexid_stop_;
			EdgesExtraInfo edges_extra_info_;
			double bus_wait_time_ = 0.0;
		};

		TransportRouter MakeTransportRouter(const TransportCatalogue& catalogue, double bus_wait_time, double bus_velocity);

	}
}