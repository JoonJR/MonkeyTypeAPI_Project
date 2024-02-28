#pragma once

#include <sqlite3.h>
#include "typingTestResults.h"
#include <vector>

class DatabaseManager {
	sqlite3* db;
	std::string formatTimestamp(long long milliseconds);
public:
	DatabaseManager(const std::string& dbName);
	~DatabaseManager();

	void createTable();
	void insertData(const TypingTestResult& result);
	bool resultExists(const TypingTestResult& result);
	std::vector<TypingTestResult> fetchResults();
};