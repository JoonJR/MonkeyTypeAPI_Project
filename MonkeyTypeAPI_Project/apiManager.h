#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "plotData.h"


class APIManager {
public:
    std::string fetchData();
    void parseData(const std::string& apiResponse, PlotData& plotData);
    void fetchParseAndInsertData();

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
};
