#include "apiManager.h"
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>

using namespace std; 

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
	userp->append((char*)contents, size * nmemb);
	return size * nmemb;
}

string APIManager::fetchData() {
	CURL* curl;
	CURLcode res;
	string readBuffer;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if (curl) {
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

		string authHeader = "Authorization: ApeKey " + string(apiKey);
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, authHeader.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, "https://api.monkeytype.com/results/last");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		if (apiKey) free(apiKey);
	}
	curl_global_cleanup();

	return readBuffer;
}


