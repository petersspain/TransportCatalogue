#pragma once
#include "geo.h"

#include <string>
#include <string_view>
#include <vector>
#include <set>

namespace catalogue {

	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<const Stop*> stops;
		bool is_roundtrip = false;
	};

	struct BusInfo {
		int stops = 0;
		int unique_stops = 0;
		int route_length = 0;
		double curvature = 0.0;
	};

	using StopInfo = const std::set<std::string_view>*;
}