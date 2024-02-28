#pragma once

#include <string>

class TypingTestResult {
public:
	double wpm;
	double rawWpm;
	double accuracy;
	std::string timestamp;
	std::string mode;
	std::string mode2;
	int correctChars;
	int incorrectChars;
	int extraChars;
	int missedChars;
	int restartCount;
	int testDuration;

	TypingTestResult(double wpm, double rawWpm, double accuracy, std::string timestamp,
					std::string mode, std::string mode2, int correctChars, int incorrectChars, int extraChars, int missedChars, int restartCount, int testDuration) :
		wpm(wpm), rawWpm(rawWpm), accuracy(accuracy), timestamp(timestamp),
		mode(mode), mode2(mode2), correctChars(correctChars), incorrectChars(incorrectChars), extraChars(extraChars),
		missedChars(missedChars), restartCount(restartCount), testDuration(testDuration) {}
};