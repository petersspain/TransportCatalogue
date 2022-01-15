#include "transport_catalogue.h"

#include <list>
#include <unordered_set>
#include <optional>

namespace catalogue {

	void TransportCatalogue::AddBus(Bus bus_to_move) {
		//ComputeDistances
		int L = 0;
		double geo = 0.0;

		geo::Coordinates last_coord = { 0.0, 0.0 },
			curr_coord = { 0.0, 0.0 };
		bool first = true;
		const Stop* last_stop = nullptr;
		for (const Stop* stop : bus_to_move.stops) {

			if (first) {
				first = false;
				last_coord = stop->coordinates;
				last_stop = stop;
			}
			else {
				curr_coord = stop->coordinates;
				L += GetDistance(last_stop->name, stop->name);
				geo += ComputeDistance(last_coord, curr_coord);
				last_coord = curr_coord;
				last_stop = stop;
			}
		}
		buses_.push_back(std::move(bus_to_move));
		const Bus* bus = &buses_.back();
		buses_by_names_[bus->name] = bus;

		//BusInfo
		for (const Stop* stop : bus->stops) {
			stops_buses_[stop->name].insert(bus->name);
		}

		//count unique stops
		std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());

		bus_info_by_name_.emplace(bus->name, BusInfo{ static_cast<int>(bus->stops.size()), static_cast<int>(unique_stops.size()), L, static_cast<double>(L) / geo });
	}

	void TransportCatalogue::AddStop(Stop stop) {
		stops_.push_back(std::move(stop));
		stops_by_names_.emplace(stops_.back().name, &stops_.back());
	}

	const Bus* TransportCatalogue::GetBusByName(std::string_view bus_name) const {
		return buses_by_names_.at(bus_name);
	}

	const Stop* TransportCatalogue::GetStopByName(std::string_view stop_name) const {
		return stops_by_names_.at(stop_name);
	}

	const BusInfo* TransportCatalogue::GetBusInfo(std::string_view bus_name) const
	{
		if (bus_info_by_name_.count(bus_name) == 0) {
			return nullptr;
		}
		return &bus_info_by_name_.at(bus_name);
	}

	std::optional<StopInfo> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
		if (stops_buses_.count(stop_name) == 0) {
			if (stops_by_names_.count(stop_name) == 0) {
				return std::nullopt;
			}
			else {
				return nullptr;
			}
		}
		return &stops_buses_.at(stop_name);
	}

	void TransportCatalogue::AddDistance(std::string_view from_name, std::string_view to_name, uint32_t distance) {
		stops_distances_[{GetStopByName(from_name), GetStopByName(to_name)}] = distance;
	}

	uint32_t TransportCatalogue::GetDistance(std::string_view from_name, std::string_view to_name) const {
		const Stop* from = GetStopByName(from_name),
			* to = GetStopByName(to_name);
		if (stops_distances_.count({ from, to }) != 0) {
			return stops_distances_.at({ from, to });
		}
		if (stops_distances_.count({ to, from }) != 0) {
			return stops_distances_.at({ to, from });
		}
		return {};
	}

	const std::deque<Bus>& TransportCatalogue::GetBuses() const {
		return buses_;
	}
	const std::deque<Stop>& TransportCatalogue::GetStops() const
	{
		return stops_;
	}
	size_t TransportCatalogue::GetStopsSize() const
	{
		return stops_.size();
	}

	const std::unordered_map<std::pair<const Stop*, const Stop*>, uint32_t, PairHasher>& TransportCatalogue::GetStopsDistances() const{
		return stops_distances_;
	}

}