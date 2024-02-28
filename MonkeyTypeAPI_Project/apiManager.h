#pragma once

#include <string>
#include <nlohmann/json.hpp>

class APIManager {
public:
	std::string fetchData();
};