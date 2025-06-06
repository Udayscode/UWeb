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
#include "session.hpp"
#include "cookie.hpp"
#include "user_store.hpp"
#include "session_middleware.hpp"
#include "controller.hpp"
#include "router.hpp"
#include "template.hpp"

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
    if (endsWith(path, ".ico")) return "image/x-icon";
    return "text/plain";
}

string readFile(const string& path) {
    cout << "[DEBUG] readFile trying to open: " << path << endl;
    ifstream file(path, ios::binary);
    if(!file.is_open()) {
        cerr << "[ERROR] Could not open file: " << path << " errno: " << strerror(errno) << endl;
        return "";
    }

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return content;
}

void serverStaticFile(Request& req, Response& res) {
    cout << "[DEBUG] serverStaticFile called for path: " << req.getPath() << endl;
    string requestedPath = req.getPath();

    if (requestedPath.empty() || requestedPath == "/") {
        requestedPath = "/index.html";
    }

    string filePath = "./public" + (requestedPath.front() == '/' ? requestedPath : "/" + requestedPath);

    cout << "[DEBUG] Serving file: " << filePath << endl;

    string fileData = readFile(filePath);
    string contentType = getMimeType(filePath);

    if (!fileData.empty()) {
        res.setStatus(200, "OK");
        res.setHeader("Content-Type", contentType);
        res.setHeader("Content-Length", to_string(fileData.size()));
        res.setBody(fileData);
    } else {
        res.setStatus(404, "Not Found");
        res.setHeader("Content-Type", "text/html");
        res.setBody("<h1>404 Not Found</h1><p>The requested resource could not be found.</p>");
    }

    res.send();
}

int main() {
    int server_fd, new_socket;
    sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[4096] = {0};

    HomeController homeController;
    UserController userController;
    SessionManager sessionManager;
    UserStore userStore;

    Router router;

    router.use([](Request& req, Response& res){
        return loggingMiddleware(req, res);
    });

    router.use([](Request& req, Response& res){
        return corsMiddleware(req, res);
    });

    router.use([](Request& req, Response& res){
        return sessionMiddleware(req, res);
    });

    router.use([](Request& req, Response& res){
        return rateLimitMiddleware(req, res);
    });

    // Home Routes
    router.get("/", [&](Request& req, Response& res) {
        homeController.index(req, res);
    });

    router.get("/about", [&](Request& req, Response& res) {
        homeController.about(req, res);
    });

    // User Routes
    router.get("/login", [&](Request& req, Response& res) {
        userController.showLoginForm(req, res);
    });

    router.post("/login", [&](Request& req, Response& res) {
        userController.login(req, res);
    });

    router.get("/logout", [&](Request& req, Response& res) {
        userController.logout(req, res);
    });

    // Protected Routes with Authentication middlewares
    vector<MiddlewareFunc> authMiddlewares = {
        [](Request& req, Response& res) {
            return authMiddleware(req, res);
        }
    };

    router.get("/profile", [&](Request& req, Response& res) {
        userController.profile(req, res);
    }, authMiddlewares);

    router.get("/dashboard", [&](Request& req, Response& res) {
        json data = {
            {"title", "dashboard"},
            {"message", "Welcome to your Dashboard"},
            {"user", {
                {"username", req.getSessionId()}
            }}
        };

        Template tmpl("dashboard.html");
        string renderedHtml = tmpl.render(data);
        
        res.setStatus(200, "OK");
        res.setHeader("Content-Type", "text/html");
        res.setBody(renderedHtml);
        res.send();
    }, authMiddlewares);

    // API endpoints
    router.get("/api/user", [&](Request& req, Response& res) {
        string sessionId = req.getCookie("session_id");
        Session* session = sessionManager.getSession(sessionId);

        if (session) {
            json responseJson = {
                {"status", "success"},
                {"loggedIn", true},
                {"username", session->username},
                {"userId", session->userId}
            };

            res.setStatus(200, "OK");
            res.setHeader("Content-Type", "application/json");
            res.setBody(responseJson.dump());
        } else {
            json responseJson = {
                {"status", "success"},
                {"loggedIn", false}
            };

            res.setStatus(200, "OK");
            res.setHeader("Content-Type", "application/json");
            res.setBody(responseJson.dump());
        }

        res.send();
    });

    router.get("/*", [&](Request& req, Response& res){
        cout << "[DEBUG] Static file route matched: " << req.getPath() << endl;
        serverStaticFile(req, res);
    });

    // 1. Creating socket
    cout << "[*] Creating socket...\n";
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
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

    cout << "[*] UWeb Framework started successfully!\n";
    cout << "[*] Visit http://localhost:8080/ in your browser\n";
    
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
        if (valread <= 0) {
            close(new_socket);
            continue;
        }

        string rawRequest(buffer, valread);
        Request req(rawRequest);
        Response res(new_socket);
        
        cout << "[*] Recieved request:\n" << rawRequest << endl;

        router.handleRequest(req, res);
        
        memset(buffer, 0, 4096);
    }
    
    close(server_fd);
    return 0;
}
// g++ -std=c++17 src/server.cpp src/request.cpp -o server && ./server
//curl -v http://localhost:8080/