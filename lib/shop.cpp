#include "shop.h"
#include <cstdlib>
#include <ctime>

Shop::Shop() {
    srand(time(nullptr));
}

std::string Shop::generateToken() {
    std::string token;
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 32; i++) {
        token += charset[rand() % (sizeof(charset) - 1)];
    }
    return token;
}

std::string Shop::loginUser(const std::string& name, int initialCoins) {
    auto nameIt = name_to_token.find(name);
    if (nameIt != name_to_token.end()) {
        // Пользователь уже залогинен
        return "";
    }
    
    std::string token = generateToken();
    User& user = users[token];
    user.name = name;
    user.coins = initialCoins;
    user.snowflakes = 0;
    user.trees = 0;
    user.goldenTickets = 0;
    user.token = token;
    name_to_token[name] = token;
    
    return token;
}

User* Shop::getUserByToken(const std::string& token) {
    auto it = users.find(token);
    if (it != users.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Shop::buySnowflake(const std::string& token) {
    auto it = users.find(token);
    if (it == users.end()) {
        return false;
    }
    
    User* user = &it->second;
    if (user->coins >= 10) {
        user->snowflakes += 1;
        user->coins -= 10;
        return true;
    }
    
    return false;
}

bool Shop::buyTree(const std::string& token) {
    auto it = users.find(token);
    if (it == users.end()) {
        return false;
    }
    
    User* user = &it->second;
    if (user->coins >= 10) {
        user->trees += 1;
        user->coins -= 10;
        return true;
    }
    
    return false;
}

bool Shop::resetUser(const std::string& token) {
    auto it = users.find(token);
    if (it == users.end()) {
        return false;
    }
    
    User* user = &it->second;
    user->coins = 10;
    user->snowflakes = 0;
    user->trees = 0;
    user->goldenTickets = 0;
    
    return true;
}

bool Shop::buyGoldenTicket(const std::string& token) {
    auto it = users.find(token);
    if (it == users.end()) {
        return false;
    }
    
    User* user = &it->second;
    if (user->snowflakes >= 1 && user->trees >= 1) {
        user->snowflakes -= 1;
        user->trees -= 1;
        user->goldenTickets += 1;
        return true;
    }
    
    return false;
}

