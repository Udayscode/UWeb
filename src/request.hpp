#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
using namespace std;

class Request {

private:
    string method;
    string path;
    map<string, string> headers;
    string body;
    map<string, string> cookies;
    map<string, string> query;
    map<string, string> formData;
    string sessionId;
    
public:

    Request(const string& rawRequest);

    string getMethod() const { return method; }
    string getPath() const { return path; }

    string getHeader(const string& key) const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : "";
    }

    string getBody() const { return body; }

    string getCookie(const string& name) const {
        auto it = cookies.find(name);
        return (it != cookies.end()) ? it->second : "";
    }

    string getFormData(const string& key) const {
        auto it = formData.find(key);
        return (it != formData.end()) ? it->second : "";
    }

    string setQuery(const string& key) const {
        auto it = query.find(key);
        return (it != query.end()) ? it->second : "";
    }

    void setSessionId (const string& id) { sessionId = id; }
    string getSessionId() const { return sessionId; }

private:
    void parseRequest(const string& rawRequest);
    void parseCookies();
    void parseQuery();
    void parseFromData();
};

#endif