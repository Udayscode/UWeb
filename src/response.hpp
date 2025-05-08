#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "cookie.hpp"

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

    void setCookie(const Cookie& cookie) {
        cookies.push_back(cookie);
    }

    void setCookie(const string& name, const string& value, int maxAgeSeconds = 3600) {
        Cookie cookie;
        cookie.name = name;
        cookie.value = value;
        cookie.expires = time(nullptr) + maxAgeSeconds
        cookies.push_back(cookie);
    }

    void redirect(const string& location) {
        statusCode = 302;
        statusMessage = "Found";
        headers["Location"] = location;
    }

    void json(const string& jsonContent) {
        headers["Content-Type"] = "application/json";
        body = jsonContent;
    }
    
    void send() {
        stringstream responseStream;

        responseStream << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
        
        headers["Content-Length"] = to_string(body.length());
        if (headers.find("Content-Type") == headers.end()) {
            headers["Content-Type"] = "text/plain";
        }

        for (const auto& pair : headers) {
            responseStream << pair.first << ": " << pair.second << "\r\n";
        }

        for (const auto& cookie : cookies) {
            responseStream << "Set-Cookie: " << cookie.toString() << "\r\n";
        }

        responseStream << "\r\n";
        responseStream << body;

        string finalResponse = responseStream.str();

        cout << "[*] Sending Response:\n" << finalResponse << endl;
        
        ::send(clientSocket, finalResponse.c_str(), finalResponse.length(), 0);

        cout << "[*] Closing connections...\n";
        
        close(clientSocket);
    }
};

#endif