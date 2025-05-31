#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <string>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>
#include "request.hpp"
#include "response.hpp"
#include "template.hpp"

using json = nlohmann::json;
using namespace std;

extern SessionManager sessionManager;
extern UserStore userStore;

class Controller {
protected:
    void render(Response& res, const string& templateFile, const json& data = {}) {
        Template tmpl(templateFile);
        string renderedHtml = tmpl.render(data);

        res.setStatus(200, "OK");
        res.setHeader("Content-Type", "text/html");
        res.setBody(renderedHtml);
        res.send();
    }

    void sendJson(Response& res, const json& data, int statusCode = 200) {
        res.setStatus(statusCode, "");
        res.setHeader("Content-Type", "application/json");
        res.setBody(data.dump());
        res.send();
    }

    void redirect(Response& res, const string& location) {
        res.redirect(location);
    }

public:
    virtual ~Controller() = default;
};

class HomeController : public Controller {
public:
    void index(Request& req, Response& res) {
        json data = {
            {"title", "Welcome to UWeb Framework"},
            {"message", "This is the homepage rendered with our template engine!"},
            {"user", {
                {"isLoggedIn", !req.getSessionId().empty()},
                {"username", "Guest"}
            }}
        };

        if (!req.getSessionId().empty()) {
            auto session = sessionManager.getSession(req.getSessionId());
            if (session) {
                data["user"]["username"] = session->username;
            }
        }

        render(res, "home/index.html", data);
    }

    void about(Request& req, Response& res) {
        json data = {
            {"title", "About UWeb Framework"},
            {"features", json::array({
                "HTTP Request/Response handling",
                "Routing system",
                "Session management",
                "Cookie support",
                "Templating engine",
                "MVC architecture"
            })}
        };

        render(res, "home/about.html", data);
    }
};

class UserController : public Controller {
public:
    void showLoginForm(Request& req, Response& res) {
        if (!req.getSessionId().empty()) {
            redirect(res, "/dashboard.html");
            return;
        }

        json data = {
            {"title", "Login"},
            {"error", false},
            {"errorMessage", ""}
        };

        render(res, "user/login.html", data);
    }

    void login(Request& req, Response& res) {
        string username = req.getFormData("username");
        string password = req.getFormData("password");

        cout << "[DEBUG] Login attempt: " << username << endl;

        bool authenticated = userStore.authenticateUser(username, password);
        
        string contentType = req.getHeader("Content-Type");
        bool isJsonRequest = (contentType.find("application/json") != string::npos);

        if (authenticated) {

            cout << "[DEBUG] User Authenticated: " << username << endl;
            User* user = userStore.getUserByUsername(username);
            string sessionId = sessionManager.createSession(user->id, user->username);

            Cookie sessionCookie;
            sessionCookie.name = "session_id";
            sessionCookie.value = sessionId;
            sessionCookie.expires = time(nullptr);
            sessionCookie.httpOnly = true;

            res.setCookie(sessionCookie);
            
            if (isJsonRequest || req.getHeader("Accept").find("application/json") != string::npos) {
                json responseJson = {
                    {"success", true},
                    {"message", "Login successful"},
                    {"redirect", "/dashboard.html"}
                };
                cout << "[DEBUG] Sending JSON Response" << endl;
                sendJson(res, responseJson);
            } else {
                cout << "[DEBUG] Rendering Dashboard " << endl;
                redirect(res, "/dashboard.html");
                cout << "[DEBUG] Dashboard Rendered " << endl;
            }
        } else {
            if (isJsonRequest || req.getHeader("Accept").find("application/json") != string::npos) {
                json responseJson = {
                    {"success", false},
                    {"message", "Invalid username or password"}
                };
                sendJson(res, responseJson, 401);
            } else {
                json data = {
                    {"title", "Login"},
                    {"error", true},
                    {"errorMessage", "Invalid username or password"}
                };
                render(res, "user/login.html", data);
            }
        }
    }

    void logout(Request& req, Response& res) {
        string sessionId = req.getCookie("session_id");
        if (!sessionId.empty()) {
            sessionManager.destroySession(sessionId);
        }

        Cookie sessionCookie;
        sessionCookie.name = "session_id";
        sessionCookie.value = "";
        sessionCookie.expires = 1;

        res.setCookie(sessionCookie);
        redirect(res, "/login.html");
    }

    void profile(Request& req, Response& res) {
        string sessionId = req.getCookie("session_id");
        if (sessionId.empty()) {
            redirect(res, "/login.html");
            return;
        }

        Session* session = sessionManager.getSession(sessionId);
        if (!session) {
            redirect(res, "/login.html");
            return;
        }

        User* user = userStore.getUserByUsername(session->username);
        if (!user) {
            redirect(res, "login.html");
            return;
        }

        json data = {
            {"title", "User Profile"},
            {"user", {
                {"username", user->username},
                {"id", user->id}
            }}
        };

        render(res, "user/profile.html", data);
    }
};

#endif