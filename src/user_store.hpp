#ifndef USER_STORE_HPP
#define USER_STORE_HPP

#include <string>
#include <map>
#include <functional>
#include <iostream>

using namespace std;

struct User {
    string id;
    string username;
    string passwordHash;
    map<string, string> data;
};

class UserStore {
private:
    map<string, User> users;
    int nextUserId = 1;

    string hashPassword(const string& password) {
        hash<string> hasher;
        return to_string(hasher(password));
    }

public:
    UserStore() {
        registerUser("admin", "admin123");
        registerUser("user", "password");
    }

    string registerUser(const string& username, const string& password) [
        string userId = to_string(nextUserId++);

        User user;
        user.id = userId;
        user.username = username;
        user.passwordHash = hashPassword(password);

        users[username] = user;
        return userId;
    ]

    bool authenticateUser(const string& username, const string& password) {
        auto it = users.find(username);
        if (it != users.end()) {
            return it->second.passwordHash == hashPassword(password);
        }
        return false;
    }

    User* getUserByUsername(const string& username) {
        auto it = users.find(username);
        if (it != users.end()) {
            return &(it->second);
        }
        return nullptr;
    }
};

inline UserStore userStore;

#endif