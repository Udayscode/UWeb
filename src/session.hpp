#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <map>
#include <ctime>
#include <random>
#include <sstream>
#include <iomanip>

using namespace std;

struct Session {
    string userId;
    string username;
    time_t expireAt;
    map<string, string> data;
};

class SessionManager {
private:
    map<string, Session> sessions;
    const int SESSION_EXPIRY = 3600;

    string generateSessionId() {
        static random_device rd;
        static mt19937 gen(rd());
        static uniform_int_distribution<> dis(0, 15);

        stringstream ss;
        ss << hex;

        for (int i=0; i<32; i++) {
            ss << dis(gen);
        }

        return ss.str();
    }

public:
    string createSession(const string& userId, const string& username) {
        string sessionId = generateSessionId();

        Session session;
        session.userId = userId;
        session.username = username;
        session.expireAt = time(nullptr) + SESSION_EXPIRY;

        sessions[sessionId] = session;

        return sessionId;
    }

    Session* getSession(const string& sessionId) {
        auto it = sessions.find(sessionId);
        if (it != sessions.end()) {
            if (time(nullptr) > it.second.expireAt) {
                sessions.erase(it);
                return nullptr;
            }
            return &(it->second);
        }
        return nullptr;
    }

    void refreshSession(const string& sessionId) {
        auto it = sessions.find(sessionId);
        if (it != sessions.end()) {
            it->second.expireAt = time(nullptr) + SESSION_EXPIRY;
        }
    }

    void destroySession(const string& sessionId) {
        sessions.erase(sessionId);
    }

    void setSessionData(const string& sessionId, const string& key, const string& value) {
        auto it = sessions.find(sessionId);
        if (it != sessions.end()) {
            it->second.data[key] = value;
        }
    }

    string getSessionData(const string& sessionId, const string& key) {
        auto it = sessions.find(sessionId);
        if (it != sessions.end() && it->second.data.find(key) != it->second.data.end()) {
            return it->second.data[key];
        }
        return "";
    }
};

inline SessionManager sessionManager;
#endif;