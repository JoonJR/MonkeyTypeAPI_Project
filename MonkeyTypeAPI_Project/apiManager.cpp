#include "apiManager.h"
#include "typingTestResults.h"
#include "databaseManager.h"
#include "plotData.h"
#include <sqlite3.h>
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>

using namespace std;


// Define the callback function to handle API response
size_t APIManager::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}
// Forward declaration
bool InsertDataIntoDatabase(sqlite3* db, float id, float wpm, float rawWPM, float accuracy, const std::string& timestamp, const std::string& mode, const std::string& mode2, int correctChars, int incorrectChars, int extraChars, int missedChars, int restartCount, int testDuration);

// Fetch data from the API endpoint
string APIManager::fetchData() {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        // Retrieve API key from environment variable
        char* apiKey = nullptr;
        size_t size = 0;
        errno_t err = _dupenv_s(&apiKey, &size, "ApeKey");
        if (err || apiKey == nullptr) {
            cerr << "API key environment variable not set!" << endl;
            if (apiKey) free(apiKey); // Ensure apiKey is freed if it was allocated
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return ""; // Early return to avoid using uninitialized apiKey
        }

        // Construct authorization header
        string authHeader = "Authorization: ApeKey " + string(apiKey);

        // Set up HTTP headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the URL of the API endpoint
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.monkeytype.com/results/last");

        // Set callback function for writing response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the HTTP request
        res = curl_easy_perform(curl);
        long http_status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

        if (res != CURLE_OK || http_status != 200) {
            cerr << "Request failed, curl error: " << curl_easy_strerror(res)
                << ", HTTP status code: " << http_status << endl;
            readBuffer = "Request error"; // Indicate error
        }
        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        if (apiKey) free(apiKey);
    }
    curl_global_cleanup();

    return readBuffer;
}

void APIManager::parseData(const string& apiResponse, PlotData& plotData) {
    // Parse the API response and populate the PlotData struct
    stringstream ss(apiResponse);
    string line;
    while (getline(ss, line)) {
        // Parse each line of the response and extract relevant information
        istringstream iss(line);
        float id, wpm, rawWPM, accuracy;
        string timestamp, mode, mode2;
        int correctChars, incorrectChars, extraChars, missedChars, restartCount, testDuration;
        if (iss >> id >> wpm >> rawWPM >> accuracy >> timestamp >> mode >> mode2 >> correctChars >> incorrectChars >> extraChars >> missedChars >> restartCount >> testDuration) {
            // Use the extracted information to populate the plotData struct
            plotData.ids.push_back(id);
            plotData.wpms.push_back(wpm);
            plotData.rawWPMs.push_back(rawWPM);
            plotData.accuracies.push_back(accuracy);
            plotData.timestamps.push_back(timestamp);
            plotData.modes.push_back(mode);
            plotData.modes2.push_back(mode2);
            plotData.correctChars.push_back(correctChars);
            plotData.incorrectChars.push_back(incorrectChars);
            plotData.extraChars.push_back(extraChars);
            plotData.missedChars.push_back(missedChars);
            plotData.restartCounts.push_back(restartCount);
            plotData.testDurations.push_back(testDuration);
        }
    }
}


void APIManager::fetchParseAndInsertData() {
    // Open the SQLite database connection
    sqlite3* db;
    char* errMsg = nullptr;

    if (sqlite3_open("monkeytype.db", &db) != SQLITE_OK) {
        cerr << "Failed to open the database." << std::endl;
        return;
    }

    // Fetch data from the API
    string jsonData = fetchData();

    // Parse JSON response
    auto jsonResponse = nlohmann::json::parse(jsonData);

    // Check if the response contains the expected message indicating success
    if (jsonResponse.contains("message") && jsonResponse["message"] == "Result retrieved") {
        // Extract the required data from the JSON response
        double wpm = jsonResponse["data"]["wpm"].get<double>();
        double rawWpm = jsonResponse["data"]["rawWpm"].get<double>();
        double accuracy = jsonResponse["data"]["acc"].get<double>();
        string mode = jsonResponse["data"]["mode"].get<string>();
        string mode2 = jsonResponse["data"]["mode2"].get<string>();

        vector<int> charStats = jsonResponse["data"]["charStats"].get<vector<int>>();
        int correctChars = charStats.size() > 0 ? charStats[0] : 0;
        int incorrectChars = charStats.size() > 1 ? charStats[1] : 0;
        int extraChars = charStats.size() > 2 ? charStats[2] : 0;
        int missedChars = charStats.size() > 3 ? charStats[3] : 0;

        int restartCount = jsonResponse["data"].contains("restartCount") ? jsonResponse["data"]["restartCount"].get<int>() : 0;
        int testDuration = jsonResponse["data"]["testDuration"].get<int>();

        // Convert the timestamp from a UNIX epoch in milliseconds to a human-readable format
        string timestamp = to_string(jsonResponse["data"]["timestamp"].get<long long>());

        // Initialize the database manager and create the table if it doesn't already exist
        DatabaseManager dbManager("monkeytype.db");
        dbManager.createTable();

        // Create a TypingTestResult object with the fetched data
        TypingTestResult testResult(wpm, rawWpm, accuracy, timestamp, mode, mode2, correctChars, incorrectChars, extraChars, missedChars, restartCount, testDuration);

        // Insert the test result data into the database. First check if the data has already been inserted
        if (!dbManager.resultExists(testResult)) {
            dbManager.insertData(testResult);
            cout << "Data inserted successfully." << endl;
        }
        else {
            cout << "Result with the same timestamp already exists. Skipping insertion." << endl;
        }
    }
    else {
        cerr << "Failed to retrieve a valid result from MonkeyType API." << endl;
    }

    // Close the SQLite database connection
    sqlite3_close(db);
}

bool InsertDataIntoDatabase(sqlite3* db, float id, float wpm, float rawWPM, float accuracy, const string& timestamp, const string& mode, const string& mode2, int correctChars, int incorrectChars, int extraChars, int missedChars, int restartCount, int testDuration) {
    string sql = "INSERT INTO TestResults (ID, WPM, rawWPM, Accuracy, Timestamp, Mode, Mode2, CorrectChars, IncorrectChars, ExtraChars, MissedChars, RestartCount, TestDuration) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_double(stmt, 1, id);
    sqlite3_bind_double(stmt, 2, wpm);
    sqlite3_bind_double(stmt, 3, rawWPM);
    sqlite3_bind_double(stmt, 4, accuracy);
    sqlite3_bind_text(stmt, 5, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, mode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, mode2.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, correctChars);
    sqlite3_bind_int(stmt, 9, incorrectChars);
    sqlite3_bind_int(stmt, 10, extraChars);
    sqlite3_bind_int(stmt, 11, missedChars);
    sqlite3_bind_int(stmt, 12, restartCount);
    sqlite3_bind_int(stmt, 13, testDuration);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cerr << "SQL error: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}
