#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

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

        read(new_socket, buffer, 4096);
        cout << "[*] Recieved request:\n" << buffer << endl;

        // 6. Properly formatted HTTP response
        string request(buffer);
        string response;
        string requestedPath;

        size_t pos = request.find("GET ");
        if (pos != string::npos) {
            size_t end_pos = request.find(" ", pos + 4);
            requestedPath = request.substr(pos + 4, end_pos - pos - 4);
        }

        size_t headerEnd = request.find("\r\n\r\n");
        string body;

        if (headerEnd != string::npos) {
            size_t contentLengthPos = request.find("Content-Length: ");
            if (contentLengthPos != string::npos) {
                contentLengthPos += 16;
                size_t lineEnd = request.find("\r\n", contentLengthPos);
                string lenStr = request.substr(contentLengthPos, lineEnd - contentLengthPos);
                int contentLen = stoi(lenStr);

                body = request.substr(headerEnd + 4, contentLen);

                cout << "[*] POST Body: " << body << endl;
            }
        }

        if (requestedPath.empty() || requestedPath == "/") {
            requestedPath = "/index.html";
        }

        string filePath = "./public" + requestedPath;
        string fileData = readFile(filePath);

        if (!fileData.empty()) {
            string contentType = getMimeType(filePath);
            stringstream oss;
            oss << "HTTP/1.1 200 OK \r\n"
                << "Content-Type: " << contentType << "\r\n"
                << "Content-Length: " << fileData.size() << "\r\n"
                << "\r\n"
                << fileData;
            response = oss.str();
        } else {
            response = 
                "HTTP/1.1 404 Not Found \r\n" 
                "Content-Type: text/plain\r\n"
                "Content-Length: 13 \r\n"
                "\r\n"
                "404 Not Found";
        }

        // 7. Send response
        write(new_socket, response.c_str(), response.size());

        // 8. Close sockets
        cout << "[*] Closing connections...\n";
        close(new_socket);
    }
        close(server_fd);
    return 0;
}