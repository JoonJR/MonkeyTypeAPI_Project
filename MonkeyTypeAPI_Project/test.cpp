// NjVjYmY3ODc0YTFmOGYzMmQ3OWNmMjQ5LmhiMHRvdVhPSld3LUZHYVNnUjBhVHZRYVhQTHJnamZ3

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sqlite3.h>

// Callback function for handling the data returned by cURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch data from the MonkeyType API
std::string fetchData() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Authorization: ApeKey NjVjYmY3ODc0YTFmOGYzMmQ3OWNmMjQ5LmhiMHRvdVhPSld3LUZHYVNnUjBhVHZRYVhQTHJnamZ3");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.monkeytype.com/results/last");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
    return readBuffer;
}

// Function to initialize the database
sqlite3* initDB(const std::string& dbName) {
    sqlite3* db;
    int rc = sqlite3_open(dbName.c_str(), &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    }
    else {
        std::cout << "Opened database successfully\n";
    }
    return db;
}

// Function to create a table
void createTable(sqlite3* db) {
    const char* sqlCreateTable = "CREATE TABLE IF NOT EXISTS TestResults (" \
        "ID INTEGER PRIMARY KEY AUTOINCREMENT," \
        "WPM REAL," \
        "RawWPM REAL," \
        "Accuracy REAL," \
        "Timestamp TEXT);"; // Adjusted to TEXT for consistency
    char* errMsg = 0;
    int rc = sqlite3_exec(db, sqlCreateTable, 0, 0, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        std::cout << "Table created successfully\n";
    }
}

// Function to insert data into the database
void insertData(sqlite3* db, double wpm, double rawWpm, double accuracy, const std::string& timestamp) {
    char* errMsg = 0;
    std::string sqlInsert = "INSERT INTO TestResults (WPM, RawWPM, Accuracy, Timestamp) VALUES (" +
        std::to_string(wpm) + ", " +
        std::to_string(rawWpm) + ", " +
        std::to_string(accuracy) + ", '" +
        timestamp + "');";
    int rc = sqlite3_exec(db, sqlInsert.c_str(), 0, 0, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        std::cout << "Record inserted successfully\n";
        std::cout << "Inserted values - WPM: " << wpm << ", RawWPM: " << rawWpm << ", Accuracy: " << accuracy << ", Timestamp: " << timestamp << std::endl;
    }
}

/*int main() {
    std::string jsonData = fetchData();
    auto jsonResponse = nlohmann::json::parse(jsonData);

    // Check if the "message" key exists and if it's equal to "Result retrieved"
    if (jsonResponse.contains("message") && jsonResponse["message"] == "Result retrieved") {
        // Extracting the required data
        double wpm = jsonResponse["data"]["wpm"];
        double rawWpm = jsonResponse["data"]["rawWpm"];
        double accuracy = jsonResponse["data"]["acc"];
        // Since the timestamp is in milliseconds, convert it to a more readable format if necessary
        long long timestamp = jsonResponse["data"]["timestamp"]; // Keeping it as a numeric type for now

        // Convert timestamp to a string or formatted datetime as needed
        // Example placeholder for conversion (actual conversion depends on your requirements)

        sqlite3* db = initDB("monkeytype.db");
        if (db != nullptr) {
            createTable(db);
            // Ensure insertData function is updated to handle rawWpm and possibly adjusted timestamp
            insertData(db, wpm, rawWpm, accuracy, std::to_string(timestamp)); // Consider formatting timestamp
            sqlite3_close(db);
        }
    }
    else {
        std::cerr << "Failed to retrieve a valid result from MonkeyType API." << std::endl;
    }
    
    return 0;
}
*/