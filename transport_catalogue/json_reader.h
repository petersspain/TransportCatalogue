#pragma once
#include "json.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "serialization.h"

namespace reader {

catalogue::TransportCatalogue ParseBaseRequests(const json::Node& base_req);

renderer::MapRenderer ParseRenderRequests(const json::Node& render_sett);

json::Document ParseStatRequests(const RequestHandler& rh, const json::Node& stat_req);

catalogue::transport_router::TransportRouter ParseRoutingSettingsRequest(const catalogue::TransportCatalogue& tc, const json::Node& routing_settings);

catalogue::Serialization ParseSerializationSettings(const json::Node& ser_sett);

}