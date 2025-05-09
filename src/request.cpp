#include "request.hpp"
#include <sstream>
#include <string.h>
#include "cookie.hpp"

using namespace std;

Request::Request(const string& rawRequest) {
    parseRequest(rawRequest);
    parseCookies();
    parseQuery();
    parseFromData();
}

void Request::parseRequest(const string& rawRequest){
    istringstream stream(rawRequest);
    string line;

    getline(stream, line);
    istringstream firstline(line);
    firstline >> method >> path;

    size_t questionPos = path.find('?');
    if (questionPos != string::npos) {
        path = path.substr(0, questionPos);
    }

    while (getline(stream, line) && line != "\r") {

        size_t pos = line.find(": ");
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 2);

            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }

            headers[key] = value;
        }
    }

    string tempBody((istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    body = tempBody;
}

void Request::parseCookies() {
    string cookieHeader = getHeader("Cookie");
    if (!cookieHeader.empty()) {
        cookies = Cookie::parseCookieHeader(cookieHeader);

        auto it = cookies.find("session_id");
        if (it != cookies.end()) {
            sessionId = it->second;
        }
    }
}

void Request::parseQuery() {
    size_t questionPos = path.find('?');
    if (questionPos != string::npos) {
        string queryString = path.substr(questionPos + 1);
        istringstream queryStream(queryString);
        string pair;

        while (getline(queryStream, pair, '&')) {
            size_t equalsPos = pair.find('=');
            if (equalsPos != string::npos) {
                string key = pair.substr(0, equalsPos);
                string value = pair.substr(equalsPos + 1);
                query[key] = value;
            }
        }
    }
}

void Request::parseFromData() {
    if (method == "POST" && getHeader("Content-Type") == "application/x-www-form-urlencoded") {
        istringstream formStream(body);
        string pair;

        while (getline(formStream, pair, '&')) {
            size_t equalsPos = pair.find('=');
            if (equalsPos != string::npos) {
                string key = pair.substr(0, equalsPos);
                string value = pair.substr(equalsPos + 1);
                formData[key] = value;
            }
        }
    }
}