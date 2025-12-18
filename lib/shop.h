#ifndef SHOP_H
#define SHOP_H

#include <string>
#include <map>
#include <atomic>

struct User {
    std::string name;
    std::atomic<int> coins;
    std::atomic<int> snowflakes;
    std::atomic<int> trees;
    std::atomic<int> goldenTickets;
    std::string token;
    
    User() : coins(0), snowflakes(0), trees(0), goldenTickets(0) {}
};

class Shop {
private:
    std::map<std::string, User> users; // token -> user
    std::map<std::string, std::string> name_to_token; // name -> token

    std::string generateToken();

public:
    Shop();
    
    // Создать или получить пользователя по имени
    std::string loginUser(const std::string& name, int initialCoins = 10);
    
    // Получить пользователя по токену
    User* getUserByToken(const std::string& token);
    
    // Купить снежинку
    bool buySnowflake(const std::string& token);
    
    // Купить елочку
    bool buyTree(const std::string& token);
    
    // Сбросить пользователя (вернуть 10 монет, обнулить покупки)
    bool resetUser(const std::string& token);
    
    // Купить золотой билет за 1 снежинку и 1 елочку
    bool buyGoldenTicket(const std::string& token);
};

#endif // SHOP_H

