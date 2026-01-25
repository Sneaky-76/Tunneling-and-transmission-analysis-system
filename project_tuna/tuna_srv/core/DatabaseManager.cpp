#include "DatabaseManager.h"
#include <iostream>

// Gemini 3 Pro was used to help implement this file.
// Introduction to programming with C++ for Engineers by Bogusław Cyganek was used as a reference. Specifically, the chapters on file handling and STL containers, meaning Chapter 3, 4, 5, 7 and 8.

DatabaseManager::DatabaseManager(const std::string& filename) : db_file_path(filename) {
    load();
}

DatabaseManager::~DatabaseManager() {
    save();
}

void DatabaseManager::load() {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream file(db_file_path);

    if (!file.is_open()) {
        std::cout << "[Database] Info: New database file will be created upon write." << std::endl;
        return;
    }

    users.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            User u = User::deserialize(line);
            if (!u.getUsername().empty()) {
                users[u.getUsername()] = u;
            }
        }
    }
}

void DatabaseManager::save() {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ofstream file(db_file_path, std::ios::trunc); // File is truncated (overwritten)

    if (!file.is_open()) {
        std::cerr << "[Database] Error: File cannot be opened for writing." << std::endl;
        return;
    }

    for (const auto& pair : users) {
        file << pair.second.serialize() << "\n";
    }
}

bool DatabaseManager::registerUser(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(db_mutex);

    // Check if the user already exists in the map
    if (users.find(username) != users.end()) {
        return false; 
    }

    // New user is added to memory
    users[username] = User(username, password);

    // Optimization: User is appended to the file immediately
    std::ofstream file(db_file_path, std::ios::app);
    if(file.is_open()){
        file << users[username].serialize() << "\n";
    }
    
    return true;
}

bool DatabaseManager::authenticate(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(db_mutex);

    auto it = users.find(username);
    if (it != users.end()) {
        return it->second.checkPassword(password);
    }
    return false;
}