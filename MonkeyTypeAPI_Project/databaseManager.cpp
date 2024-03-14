#include "databaseManager.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>

using namespace std;

DatabaseManager::DatabaseManager(const string& dbName) {
	int rc = sqlite3_open(dbName.c_str(), &db);
	if (rc) {
		cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
	}
	else {
		cout << "\nOpened database successfully\n";
	}
}

DatabaseManager::~DatabaseManager() {
	sqlite3_close(db);
}

vector<TypingTestResult> DatabaseManager::fetchResults() {
	vector<TypingTestResult> results;
	const char* sqlQuery = "SELECT ID, WPM, rawWPM, Accuracy, Timestamp, Mode, Mode2, CorrectChars, IncorrectChars, ExtraChars, MissedChars, RestartCount, TestDuration FROM TestResults ORDER BY Timestamp DESC;";
	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(db, sqlQuery, -1, &stmt, nullptr) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			TypingTestResult result(
				
				sqlite3_column_double(stmt, 1), // WPM 
				sqlite3_column_double(stmt, 2),	// RAWWPM
				sqlite3_column_double(stmt, 3),	// Accuracy
				reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)), // Timestamp
				reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)), // Mode
				reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)), // Mode2
				sqlite3_column_int(stmt, 7), // CorrectChars
				sqlite3_column_int(stmt, 8), // IncorrectChars
				sqlite3_column_int(stmt, 9), // ExtraChars
				sqlite3_column_int(stmt, 10), // MissedChars
				sqlite3_column_int(stmt, 11), // RestartCount
				sqlite3_column_int(stmt, 12) // TestDuration
			);
			results.push_back(result);
		}
		sqlite3_finalize(stmt);
	}
	else {
		cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << endl;
	}
	return results;
}

void DatabaseManager::createTable() {
	const char* sqlCreateTable = "CREATE TABLE IF NOT EXISTS TestResults ("
		"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
		"WPM REAL, RawWPM REAL, Accuracy REAL,"
		"Timestamp TEXT, Mode TEXT, Mode2 TEXT, CorrectChars INTEGER, IncorrectChars INTEGER, ExtraChars INTEGER, MissedChars INTEGER,"
		"RestartCount INTEGER, TestDuration INTEGER);";
	char* errMsg = 0;
	int rc = sqlite3_exec(db, sqlCreateTable, 0, 0, &errMsg);
	if (rc != SQLITE_OK) {
		cerr << "SQL error: " << errMsg << endl;
		cout << "HEre1" << endl;
		sqlite3_free(errMsg);
	}
	else {
		cout << "Table created successfully\n";
	}
}

void DatabaseManager::insertData(const TypingTestResult& result) {
	string formattedTimestamp = formatTimestamp(stoll(result.timestamp));
	string sqlInsert = "INSERT INTO TestResults (WPM, RawWPM, Accuracy, Timestamp, Mode, Mode2, CorrectChars, IncorrectChars, ExtraChars, MissedChars, RestartCount, TestDuration) VALUES ("
		+ to_string(result.wpm) + ", "
		+ to_string(result.rawWpm) + ", "
		+ to_string(result.accuracy) + ", '"
		+ formattedTimestamp + "', '"
		+ result.mode + "', '"
		+ result.mode2 + "', "
		+ to_string(result.correctChars) + ", "
		+ to_string(result.incorrectChars) + ", "
		+ to_string(result.extraChars) + ", "
		+ to_string(result.missedChars) + ", "
		+ to_string(result.restartCount) + ", "
		+ to_string(result.testDuration) + ");";

	char* errMsg = 0;
	int rc = sqlite3_exec(db, sqlInsert.c_str(), 0, 0, &errMsg);
	if (rc != SQLITE_OK) {
		cerr << "SQL error: " << errMsg << endl;
		cout << "HEre2" << endl;
		sqlite3_free(errMsg);
	}
	else {
		cout << "Record inserted successfully\n";
		cout << "Inserted values - WPM: " << result.wpm << ", RawWPM: " << result.rawWpm << ", Accuracy: " << result.accuracy << ", Timestamp: " << formattedTimestamp << ", Mode: " 
			<< result.mode << ", Mode2: " << result.mode2 << ", CorrectChars: " << result.correctChars << ", IncorrectChars: " << result.incorrectChars << ", ExtraChars: " 
			<< result.extraChars << ", MissedChars: " << result.missedChars << ", restartCount: " << result.restartCount << ", testDuration: " << result.testDuration << endl;
	}
}

bool DatabaseManager::resultExists(const TypingTestResult& result) {
	// query to check if a result with the same timestamp exists
	string sqlQuery = "SELECT COUNT(*) FROM TestResults WHERE Timestamp = '" + formatTimestamp(stoll(result.timestamp)) + "';";

	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db, sqlQuery.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			int count = sqlite3_column_int(stmt, 0);
			sqlite3_finalize(stmt);
			return count > 0;
		}
	}
	sqlite3_finalize(stmt);
	return false; 
}

string DatabaseManager::formatTimestamp(long long milliseconds) {
	// Convert milliseconds to seconds
	auto seconds = chrono::seconds(milliseconds / 1000);
	auto milliseconds_remainder = milliseconds % 1000;

	// Convert to time_t
	time_t time = seconds.count();

	// Prepare tm struct
	tm gmtimeBuffer{};
	// Use gmtime_s on Windows
	gmtime_s(&gmtimeBuffer, &time);

	gmtimeBuffer.tm_hour += 2; // Add 2 hours to match local timezone

	mktime(&gmtimeBuffer);
	// Format the timestamp using the gmtimeBuffer
	stringstream ss;
	ss << put_time(&gmtimeBuffer, "%Y-%m-%d %H:%M:%S");

	

	return ss.str();
}