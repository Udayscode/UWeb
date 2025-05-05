#include "request.hpp"
#include <sstream>
#include <string.h>
using namespace std;

Request::Request(const string& rawRequest) {
    parseRequest(rawRequest);
}

void Request::parseRequest(const string& rawRequest){
    istringstream stream(rawRequest);
    string line;

    getline(stream, line);
    istringstream firstline(line);
    firstline >> method >> path;

    while (getline(stream, line) && line != "\r") {

        size_t pos = line.find(": ");
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 2);
            headers[key] = value;
        }
    }

    string tempBody((istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    body = tempBody;
}