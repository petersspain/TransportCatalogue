#include "json.h"
#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <optional>
#include <iostream>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    json::Document doc = json::Load(std::cin);

    if (mode == "make_base"sv) {
        const catalogue::TransportCatalogue& tc = reader::ParseBaseRequests(doc.GetRoot().AsDict().at("base_requests"s));
        const renderer::MapRenderer& mr = reader::ParseRenderRequests(doc.GetRoot().AsDict().at("render_settings"s));
        const catalogue::transport_router::TransportRouter& tr = reader::ParseRoutingSettingsRequest(tc, doc.GetRoot().AsDict().at("routing_settings"s));
        const catalogue::Serialization& serializer = reader::ParseSerializationSettings(doc.GetRoot().AsDict().at("serialization_settings"s));
        serializer.SerializeCatalogue(tc, mr, tr);
    } 
    else if (mode == "process_requests"sv) {
        const catalogue::Serialization& serializer = reader::ParseSerializationSettings(doc.GetRoot().AsDict().at("serialization_settings"s));
        auto data = serializer.DeserializeCatalogue();
        if (!data) {
            std::cerr << "Unable to deserialize DATABASE" << std::endl;
            return 2;
        }
        //DEFAULT ROUTER FOR THIS TASK
        const catalogue::transport_router::TransportRouter tr(data->router_settings.graph, data->router_settings.routes_internal_data, data->router_settings.stop_vertex_id, data->router_settings.vertex_id_stop, data->router_settings.edges_extra_info, data->router_settings.bus_wait_time);
        json::Document out = reader::ParseStatRequests({ data->transport_catalogue, data->map_renderer, tr }, doc.GetRoot().AsDict().at("stat_requests"s));
        json::Print(out, std::cout);
    } 
    else {
        PrintUsage();
        return 1;
    }
}