#include "APIManager.h"
#include "DatabaseManager.h"
#include <iostream>
#include <vector>
using namespace std;

int main() {
    APIManager apiManager;
    string jsonData = apiManager.fetchData(); // Fetch the JSON data from the API.
    auto jsonResponse = nlohmann::json::parse(jsonData); // Parse the JSON string into a JSON object.

    // Check if the response contains the expected message indicating success.
    if (jsonResponse.contains("message") && jsonResponse["message"] == "Result retrieved") {
        cout << jsonResponse << endl;
        // Extract the required data from the JSON response.
        double wpm = jsonResponse["data"]["wpm"]; 
        double rawWpm = jsonResponse["data"]["rawWpm"];
        double accuracy = jsonResponse["data"]["acc"];
        string mode = jsonResponse["data"]["mode"];  // Test Mode (Time, Words) 
        string mode2 = jsonResponse["data"]["mode2"]; 

        std::vector<int> charStats = jsonResponse["data"]["charStats"].get<std::vector<int>>();
        int correctChars = charStats.size() > 0 ? charStats[0] : 0;
        int incorrectChars = charStats.size() > 1 ? charStats[1] : 0;
        int extraChars = charStats.size() > 2 ? charStats[2] : 0;
        int missedChars = charStats.size() > 3 ? charStats[3] : 0;

        int restartCount = jsonResponse["data"].contains("restartCount") ? jsonResponse["data"]["restartCount"].get<int>() : 0;
        int testDuration = jsonResponse["data"]["testDuration"];

        // Convert the timestamp from a UNIX epoch in milliseconds to  human-readable format.
        // convert it to a string as-is.
        string timestamp = to_string(jsonResponse["data"]["timestamp"].get<long long>());



        // Initialize the database manager and create the table if it doesn't already exist.
        DatabaseManager dbManager("monkeytype.db");
        dbManager.createTable();

        // Create a TypingTestResult object with the fetched data.
        TypingTestResult testResult(wpm, rawWpm, accuracy, timestamp, mode, mode2, correctChars, incorrectChars, extraChars, missedChars, restartCount, testDuration); 

        // Insert the test result data into the database. First check if the data has already been inserted. 
        if (!dbManager.resultExists(testResult)) {
            dbManager.insertData(testResult);

            cout << "Data inserted successfully." << endl;
        }
        else {
            std::cout << "Result with the same timestamp already exists. Skipping insertion." << std::endl;
        }

        // fetch all results from the database and print them.
        vector<TypingTestResult> results = dbManager.fetchResults();
        cout << "Fetching results from database..." << endl;
        for (const auto& result : results) {
            cout << "WPM: " << result.wpm << ", Raw WPM: " << result.rawWpm << ", Accuracy: " << result.accuracy
                << ", Timestamp: " << result.timestamp << ", Mode: " << result.mode << ", Mode2: " << result.mode2 
                << ", CorrectChars: " << result.correctChars << ", IncorrectChars: " << result.incorrectChars
                << ", ExtraChars: " << result.extraChars << ", MissedChars: " << result.missedChars << ", RestartCount: " << result.restartCount 
                << ", TestDuration: " << result.testDuration << " seconds" << endl;
        }

        
    }
    else {
        cerr << "Failed to retrieve a valid result from MonkeyType API." << endl;
    }

    return 0;
}
 
// sqlite3 monkeytype.db  SELECT* FROM TestResults;
