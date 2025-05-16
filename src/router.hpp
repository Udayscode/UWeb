#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <regex>
#include "request.hpp"
#include "response.hpp"
#include "middleware.hpp"

using namespace std;

using RouteHandler = function<void(Request& req, Response& res)>;
using MiddlewareFunc = function<void(Request& req, Response& res)>;

struct Route {
    string method;
    string path;
    RouteHandler handler;
    regex pathRegex;
    vector<string> paramNames;
    vector<MiddlewareFunc> middlewares;
};

class Router {
private:
    vector<Route> routes;
    vector<MiddlewareFunc> globalMiddleswares;

    pair<regex, vector<string>> pathToRegex(const string& path) {
        string pattern = path;
        vector<string> paramNames;

        regex paramPattern(":(\\w+)");
        string regexPattern = "^";

        string::const_iterator searchStart(path.cbegin());
        smatch matches;
        string remainingPath = path;
        size_t lastPos = 0;

        while (regex_search(searchStart, path.cend(), matches, paramPattern)) {
            string paramName = matches[1].str();
            paramNames.push_back(paramName);

            regexPattern += remainingPath.substr(0, matches.position());
            regexPattern += "([^/]+)";

            lastPos = matches.position() + matches.length();
            remainingPath = remainingPath.substr(lastPos);
            searchStart = matches.suffix().first;    
        }

        regexPattern += remainingPath;
        regexPattern += "$";

        return {regex(regexPattern), paramNames};
    }

    bool matchRoute(const Route& route, const string& path, map<string, string>& params) {
        smatch matches;
        if (regex_match(path, matches, route.pathRegex)) {
            for (size_t i = 0; i < route.paramNames.size(); i++) {
                params[route.paramNames[i]] = matches[i + 1].str();
            }
            return true;
        }
        return false;
    }

public:
    void use(MiddlewareFunc middleware) {
        globalMiddleswares.push_back(middleware);
    }

    void addRoute(const string& method, const string& path, RouteHandler handler, const vector<MiddlewareFunc>& middlewares = {}) {
        auto [pathRegex, paramNames] = pathToRegex(path);

        Route route;
        route.method = method;
        route.path = path;
        route.handler = handler;
        route.pathRegex = pathRegex;
        route.paramNames = paramNames;
        route.middlewares = middlewares;

        routes.push_back(route);
    }

    void get(const string& path, RouteHandler handler, const vector<MiddlewareFunc>& middlewares = {}) {
        addRoute("GET", path, handler, middlewares);
    }

    void post(const string& path, RouteHandler handler, const vector<MiddlewareFunc>& middlewares = {}) {
        addRoute("POST", path, handler, middlewares);
    }

    void put(const string& path, RouteHandler handler, const vector<MiddlewareFunc>& middlewares = {}) {
        addRoute("PUT", path, handler, middlewares);
    }

    void del(const string& path, RouteHandler handler, const vector<MiddlewareFunc>& middlewares = {}) {
        addRoute("DELETE", path, handler, middlewares);
    }

    void handleRequest(Request& req, Response& res) {
        string path = req.getPath();
        string method = req.getMethod();

        for (const auto& middleware : globalMiddleswares) {
            middleware(req, res);

            if (res.isSent()) {
                return;
            }
        }

        for (const auto& route : routes) {
            if (route.method == method) {
                map<string, string> params;
                if (matchRoute(route, path, params)) {
                    for (const auto& param : params) {
                        req.addParam(param.first, param.second);
                    }

                    for (const auto& middleware : middlewares) {
                        middleware(req, res);

                        if(res.isSent()) {
                            return;
                        }
                    }

                    route.handler(req, res);
                    return;
                }
            }
        }

        res.setStatus(404, "Not Found");
        res.setHeader("Content-Type", "text/html");
        res.setBody("<h1>404 - Not Found</h1><p>The requested resource could not be found.</p>");
        res.send();
    }
};

#endif