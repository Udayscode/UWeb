#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;

class Response {

private:
    int clientSocket;
    int statusCode = 200;
    string statusMessage = "OK";
    map<string, string> headers;
    string body;

public: 

    Response(int sock) : clientSocket(sock){}

    void setStatus(int code, const string& message) {
        statusCode = code;
        statusMessage = message;
    }

    void setHeader(const string& key, const string& value) {
        headers[key] = value;
    }

    void setBody(const string& content) {
        body = content;
    }
    
    void send() {
        stringstream responseStream;

        responseStream << "HTTP/1.1" << statusCode << " " << statusMessage << "\r\n";
        headers["Content-Length"] = to_string(body.length());
        if (headers.find("Content-Type") == headers.end()) {
            headers["Content-Type"] = "text/plain";
        }

        for (const auto& pair : headers) {
            responseStream << pair.first << ": " << pair.second << "\r\n";
        }

        responseStream << "\r\n";
        responseStream << body;

        string finalResponse = responseStream.str();
        ::send(clientSocket, finalResponse.c_str(), finalResponse.length(), 0);
    }
};

#endif