#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP

#include "request.hpp"
#include "response.hpp"
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

void loggingMiddleware(Request& req, Response& res) {
    cout << "[LOG] " << req.getMethod() << " " << req.getPath() << endl;
}

void authMiddleware(Request& req, Response& res) {
    string token = req.getHeader("Authorization");
    if (token != "Bearer secret123") {
        res.setStatus(401, "");
        res.setBody("Unauthorized Access");
        res.send();
    }
}

void corsMiddleware(Request& req, Response& res) {
    res.setHeader("Access-Control-Allow-Origin", "*");
    res.setHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

    if  (req.getMethod() == "OPTIONS") {
        res.setStatus(200, "");
        res.send();
    }
}

unordered_map<string, int> requestCount;

void rateLimitMiddleware(Request& req, Response& res) {
    string ip = req.getHeader("X-Forwarded-For");
    requestCount[ip]++;

    if (requestCount[ip] > 100) {
        res.setStatus(429, "");
        res.setBody("Too Many Requests");
        res.send();
    }
}

#endif