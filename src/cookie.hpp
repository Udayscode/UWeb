#ifndef COOKIE_HPP
#define COOKIE_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>

using namespace std;

class Cookie {
public:
    string name;
    string value;
    time_t expires = 0;
    string path = "/";
    string domain = "";
    bool secure = false;
    bool httpOnly = true;
    string sameSite = "Lax";

    string toString() const {
        stringstream ss;
        ss << name << "=" << value;

        if (!path.empty()) {
            ss << "; Path=" << path;
        }

        if (!domain.empty()) {
            ss << "; Domain=" << domain;
        }

        if (expires > 0) {
            char timeBuffer[100];
            struct tm* timeinfo = gmtime(&expires);
            strftime(timeBuffer, sizeof(timeBuffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
            ss << "; Expires=" << timeBuffer;
        }

        if (secure) {
            ss << "; Secure";
        }

        if (httpOnly) {
            ss << "; HttpOnly";
        }

        if (!sameSite.empty()) {
            ss << "; SameSite=" << sameSite;
        }

        return ss.str();
    }

    static map<string, string> parseCookieHeader(const string& cookieHeader) {
        map<string, string> cookies;
        istringstream stream(cookieHeader);
        string pair;

        while (getline(stream, pair, ';')) {
            size_t pos = pair.find('=');
            if (pos != string::npos) {
                string key = pair.substr(0, pos);
                string value = pair.substr(pos + 1);

                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                cookies[key] = value;
            }
        }

        return cookies;
    }
};

#endif