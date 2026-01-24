#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

// Gemini 3 Pro was used to help implement this file.

#include "User.h"
#include <map>
#include <string>
#include <mutex>
#include <fstream>

/*
 * Manages user persistence and authentication.
 * Provides thread-safe access to user data.
 */
class DatabaseManager {
private:
    const std::string db_file_path;
    std::map<std::string, User> users; // In-memory storage: Key=username, Value=User object
    std::mutex db_mutex;               // Mutex ensuring thread safety

    // Loads data from the file into memory
    void load();
    
    // Saves all data from memory to the file
    void save();

public:
    // Constructor initializes the database with a specific file path
    explicit DatabaseManager(const std::string& filename);
    
    // Destructor ensures data is saved upon shutdown
    ~DatabaseManager();

    // Registers a new user. Returns false if the username already exists.
    bool registerUser(const std::string& username, const std::string& password);

    // Authenticates a user. Returns true if credentials are valid.
    bool authenticate(const std::string& username, const std::string& password);
};

#endif