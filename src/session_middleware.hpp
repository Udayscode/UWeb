#ifndef SESSION_MIDDLEWARE_HPP
#define SESSION_MIDDLEWARE_HPP

#include "request.hpp"
#include "response.hpp"
#include "session.hpp"
#include "cookie.hpp"

void sessionMiddleware(Request& req, Response& res) {
    string sessionId = req.getCookie("session_id");

    if (!sessionId.empty()) {
        Session* session = sessionManager.getSession(sessionId);
        if (session) {
            sessionManager.refreshSession(sessionId);
            req.setSessionId(sessionId);
        }
    }
}

bool authSessionMiddleware(Request& req, Response& res) {
    string sessionId = req.getCookie("session_id");

    if (sessionId.empty()) {
        res.setStatus(401, "Unauthorised");
        res.setBody("You must be logged in to access this page");
        res.redirect("/login.html");
        res.send();
        return false;
    }

    Session* session = sessionManager.getSession(sessionId);
    if (!session) {
        res.setStatus(401, "Unuthorized");
        res.setBody("Invalid or expired session");
        res.redirect("/login.html");
        res.send();
        return false;
    }

    return true;
}

#endif