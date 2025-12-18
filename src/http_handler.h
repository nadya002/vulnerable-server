#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <string>
#include "../lib/shop.h"

class HttpHandler {
private:
    Shop& shop;
    
    std::string urlDecode(const std::string& str);
    std::string extractToken(const std::string& cookie);
    std::string readFile(const std::string& filename);
    std::string renderTemplate(const std::string& templateContent, const User& user);
    
public:
    HttpHandler(Shop& s) : shop(s) {}
    
    std::string handleRequest(const std::string& request);
};

#endif // HTTP_HANDLER_H

