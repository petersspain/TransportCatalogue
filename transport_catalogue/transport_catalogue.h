#pragma once
#include "domain.h"

#include <unordered_map>
#include <set>
#include <deque>
#include <cstdint>
#include <string_view>
#include <optional>

namespace catalogue {

	class PairHasher {
	public:
		size_t operator()(std::pair<const Stop*, const Stop*> to_hash) const {
			return static_cast<size_t>(hasher_(to_hash.first) * 37 + hasher_(to_hash.second));
		}
	private:
		std::hash<const void*> hasher_;
	};

	class TransportCatalogue {
	public:
		void AddBus(Bus bus);
		void AddStop(Stop stop);
		const Bus* GetBusByName(std::string_view bus_name) const;
		const Stop* GetStopByName(std::string_view stop_name) const;
		const BusInfo* GetBusInfo(std::string_view bus_name) const;
		std::optional<StopInfo> GetStopInfo(std::string_view stop_name) const;
		void AddDistance(std::string_view from, std::string_view to, uint32_t distance);
		uint32_t GetDistance(std::string_view from, std::string_view to) const;
		const std::unordered_map<std::pair<const Stop*, const Stop*>, uint32_t, PairHasher>& GetStopsDistances() const;

		const std::deque<Bus>& GetBuses() const;
		const std::deque<Stop>& GetStops() const;

		size_t GetStopsSize() const;
	private:
		std::deque<Stop> stops_;
		std::deque<Bus> buses_;

		std::unordered_map<std::string_view, const Stop*> stops_by_names_;
		std::unordered_map<std::string_view, const Bus*> buses_by_names_;

		std::unordered_map<std::string_view, std::set<std::string_view>> stops_buses_;

		std::unordered_map<std::pair<const Stop*, const Stop*>, uint32_t, PairHasher> stops_distances_;

		//BusInfo s
		std::unordered_map<std::string_view, BusInfo> bus_info_by_name_;
	};
}