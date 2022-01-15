#pragma once
#include <string_view>
#include <optional>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_router.h"
#include "json.h"

class RequestHandler {
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const catalogue::TransportCatalogue& db, const renderer::MapRenderer& renderer, const catalogue::transport_router::TransportRouter& router);

    // Возвращает информацию о маршруте (запрос Bus)
    const catalogue::BusInfo* GetBusStat(std::string_view bus_name) const;

    // Возвращает маршруты, проходящие через
    std::optional<catalogue::StopInfo> GetBusesByStop(std::string_view stop_name) const;

    // Возвращает получившуюся картинку в виде svg документа
    svg::Document RenderMap() const;

    json::Node Route(std::string_view, std::string_view) const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    const catalogue::transport_router::TransportRouter& router_;
};