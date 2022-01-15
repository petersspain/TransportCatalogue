#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "svg.h"
#include "graph.h"

#include <filesystem>
#include <optional>

namespace catalogue {
	class Serialization {
	public:
		struct RouterSettings {
			catalogue::transport_router::TransportRouter::Graph graph;
			graph::Router<double>::RoutesInternalData routes_internal_data;
			catalogue::transport_router::TransportRouter::StopVertexId stop_vertex_id;
			catalogue::transport_router::TransportRouter::VertexIdStop vertex_id_stop;
			catalogue::transport_router::TransportRouter::EdgesExtraInfo edges_extra_info;
			double bus_wait_time = 0.0;
		};

		struct SerializationOut {
			catalogue::TransportCatalogue transport_catalogue;
			renderer::MapRenderer map_renderer;
			RouterSettings router_settings;
		};

		explicit Serialization(const std::filesystem::path& path) 
			: filename_(path) {
		}

		void SerializeCatalogue(const catalogue::TransportCatalogue& database, const renderer::MapRenderer& mr, const catalogue::transport_router::TransportRouter&) const;

		std::optional<SerializationOut> DeserializeCatalogue() const;

	private:
		const std::filesystem::path filename_;
	};
}