#pragma once
#include <vector>
#include <string>

class PlotData {
public:
    // Constructor
    PlotData() {}

    // Destructor
    ~PlotData() {}

    // Getter methods
    std::vector<float>& getIds() { return ids; }
    std::vector<float>& getWpms() { return wpms; }
    std::vector<float>& getRawWPMs() { return rawWPMs; }
    std::vector<float>& getAccuracies() { return accuracies; }
    std::vector<std::string>& getTimestamps() { return timestamps; }
    std::vector<std::string>& getModes() { return modes; }
    std::vector<std::string>& getModes2() { return modes2; }
    std::vector<int>& getCorrectChars() { return correctChars; }
    std::vector<int>& getIncorrectChars() { return incorrectChars; }
    std::vector<int>& getExtraChars() { return extraChars; }
    std::vector<int>& getMissedChars() { return missedChars; }
    std::vector<int>& getRestartCounts() { return restartCounts; }
    std::vector<int>& getTestDurations() { return testDurations; }

    // Data members
    std::vector<float> ids;
    std::vector<float> wpms;
    std::vector<float> rawWPMs;
    std::vector<float> accuracies;
    std::vector<std::string> timestamps;
    std::vector<std::string> modes;
    std::vector<std::string> modes2;
    std::vector<int> correctChars;
    std::vector<int> incorrectChars;
    std::vector<int> extraChars;
    std::vector<int> missedChars;
    std::vector<int> restartCounts;
    std::vector<int> testDurations;

    void clear() {
        ids.clear();
        wpms.clear();
        rawWPMs.clear();
        accuracies.clear();
        timestamps.clear();
        modes.clear();
        modes2.clear();
        correctChars.clear();
        incorrectChars.clear();
        extraChars.clear();
        missedChars.clear();
        restartCounts.clear();
        testDurations.clear();
    }

    float calculateAverageWPM(const std::vector<float>& wpms) {
        if (wpms.empty()) return 0.0f;
        float sum = std::accumulate(wpms.begin(), wpms.end(), 0.0f);
        return sum / wpms.size();
    }

    std::vector<float> calculateRunningAverage(const std::vector<float>& wpms) {
        std::vector<float> runningAverages;
        float sum = 0.0f;
        for (size_t i = 0; i < wpms.size(); ++i) {
            sum += wpms[i];
            runningAverages.push_back(sum / (i + 1));
        }
        return runningAverages;
    }
};