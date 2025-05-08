#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include "request.hpp"
#include "response.hpp"
#include "middleware.hpp"

using json = nlohmann::json;
using namespace std;

map<string, string> parseFromData(const string& body) {
    map<string, string> res;
    istringstream ss(body);
    string token;
    while (getline(ss, token, '&')) {
        auto posi = token.find('=');
        if (posi != string::npos) {
            string key = token.substr(0, posi);
            string value = token.substr(posi + 1);
            res[key] = value;
        }
    }
    return res;
}

bool endsWith(const string& value, const string& ending) {
    if (ending.size() > value.size()) return false;
    return equal(ending.rbegin(), ending.rend(), value.rbegin());
}

string getMimeType(const string& path) {
    if (endsWith(path, ".html")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".png")) return "image/png";
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    if (endsWith(path, ".svg")) return "image/svg+xml";
    return "text/plain";
}

string readFile(const string& path) {
    ifstream file(path, ios::binary);
    if(!file.is_open()) return "";

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return content;
}

int main() {
    int server_fd, new_socket;
    sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[4096] = {0};

    // 1. Creating socket
    cout << "[*] Creating socket...\n";
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Setup address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // 3. Binding
    cout << "[*] Binding socket...\n";
    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. Listening
    cout << "[*] Listening...\n";
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // 5. Accepting connection
    while(true) {
        cout << "[*] Waiting for client...\n";
        new_socket = accept(server_fd, (sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        cout << "[*] Client connected, sending response...\n";

        int valread = read(new_socket, buffer, 4096);
        string rawRequest(buffer, valread);
        Request req(rawRequest);
        Response res(new_socket);
        cout << "[*] Recieved request:\n" << rawRequest << endl;

        loggingMiddleware(req, res);
        corsMiddleware(req, res);
        if (req.getMethod() == "OPTIONS") continue;

        rateLimitMiddleware(req, res);
        if (req.getBody() == "Too Many Requests") continue;

        // 6. Properly formatted HTTP response
        string response;
        string requestedPath;

        if (req.getMethod() == "POST" && req.getPath() == "/login") {

            auto formData = parseFromData(req.getBody());
            string user = formData["username"];
            string pass = formData["password"];

            json j = {
                {"status", "success"},
                {"username", user},
                {"password", pass},
                {"token", "Bearer secret123"}
            };
            string data = j.dump();
            
            res.setStatus(200, "OK");
            res.setHeader("Content-Type", "application/json");
            res.setHeader("Content-Length", to_string(data.size()));
            res.setBody(data);

            res.send();

        } else if (req.getMethod() == "GET") {        
            requestedPath = req.getPath();

            if (requestedPath == "/secret.html" || requestedPath == "/dashboard.html") {
                authMiddleware(req, res);
                if (req.getBody() == "Unauthorized Access") continue;
            }

            if (requestedPath.empty() || requestedPath == "/") {
                requestedPath = "/index.html";
            }

            string filePath = "./public" + requestedPath;
            string fileData = readFile(filePath);
            string contentType = getMimeType(filePath);

            if (!fileData.empty()) {
                res.setStatus(200, "OK");
                res.setHeader("Content-Type", contentType);
                res.setHeader("Content-Length", to_string(fileData.size()));
                res.setBody(fileData);

                res.send();
            } else {
                res.setStatus(404, "Not Found");
                res.setHeader("Content-Type", contentType);
                res.setBody("404 Not Found");

                res.send();
            }
        }

        // 8. Close sockets
        // cout << "[*] Closing connections...\n";
        // close(new_socket);
    }
        close(server_fd);
    return 0;
}
// g++ src/server.cpp src/request.cpp -o server && ./server
//curl -v http://localhost:8080/
