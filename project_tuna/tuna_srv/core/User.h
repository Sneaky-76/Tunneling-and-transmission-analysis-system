#ifndef USER_H
#define USER_H

#include <string>
#include <sstream>
#include <utility> 

/*
 * Represents a single user entity.
 * Handles serialization to and from "username:password" format.
 */
class User {
private:
    std::string username;
    std::string password;

public:
    // Default constructor
    User() = default;

    // Constructor using move semantics for string efficiency
    User(std::string u, std::string p) : username(std::move(u)), password(std::move(p)) {}

    std::string getUsername() const { return username; }
    
    // Verifies if the provided password matches the stored one
    bool checkPassword(const std::string& input_pass) const {
        return password == input_pass;
    }

    // Serializes the object to a string format: "username:password"
    std::string serialize() const {
        return username + ":" + password;
    }

    // Factory method: Deserializes string data into a User object
    static User deserialize(const std::string& line) {
        std::stringstream ss(line);
        std::string segment;
        std::string u, p;

        // Data is split by the ':' delimiter
        if(std::getline(ss, u, ':') && std::getline(ss, p)) {
            return User(u, p);
        }
        // An empty object is returned on parsing failure
        return User("", ""); 
    }
};

#endif