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
    
public:

    Request(const string& rawRequest);

    string getMethod() const { return method; }
    string getPath() const { return path; }
    string getHeader(const string& key) const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : "";
    }
    string getBody() const { return body; }

private:

    void parseRequest(const string& rawRequest);

};

#endif