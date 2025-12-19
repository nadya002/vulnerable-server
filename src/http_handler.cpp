#include "http_handler.h"
#include <sstream>
#include <fstream>
#include <iostream>

std::string HttpHandler::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream is(str.substr(i + 1, 2));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string HttpHandler::extractToken(const std::string& cookie) {
    size_t pos = cookie.find("token=");
    if (pos != std::string::npos) {
        size_t start = pos + 6;
        size_t end = cookie.find(";", start);
        if (end == std::string::npos) end = cookie.length();
        return cookie.substr(start, end - start);
    }
    return "";
}

std::string HttpHandler::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Удаляем возможные BOM и другие проблемы
    if (content.length() >= 3 && 
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content = content.substr(3);
    }
    
    return content;
}

std::string HttpHandler::renderTemplate(const std::string& templateContent, const User& user) {
    std::string result = templateContent;
    
    // Функция для замены всех вхождений
    auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };
    
    // Заменяем плейсхолдеры (сначала длинные, чтобы избежать конфликтов)
    replaceAll(result, "{{SNOWFLAKES}}", std::to_string(user.snowflakes));
    replaceAll(result, "{{USER_NAME}}", user.name);
    replaceAll(result, "{{TREES}}", std::to_string(user.trees));
    replaceAll(result, "{{COINS}}", std::to_string(user.coins));
    replaceAll(result, "{{GOLDEN_TICKETS}}", std::to_string(user.goldenTickets));
    
    // Заменяем все вхождения {{DISABLED}}
    std::string disabledValue = (user.coins < 10) ? " disabled" : "";
    replaceAll(result, "{{DISABLED}}", disabledValue);
    
    // Заменяем все вхождения {{GOLDEN_DISABLED}}
    std::string goldenDisabledValue = (user.snowflakes < 1 || user.trees < 1) ? " disabled" : "";
    replaceAll(result, "{{GOLDEN_DISABLED}}", goldenDisabledValue);
    
    // Кнопка получения секретного ключа активна только если есть золотой билет
    std::string secretKeyDisabled = (user.goldenTickets < 1) ? " disabled" : "";
    replaceAll(result, "{{SECRET_KEY_DISABLED}}", secretKeyDisabled);
    
    // Секретный ключ показывается только если он был получен (оставляем плейсхолдер пустым изначально)
    replaceAll(result, "{{SECRET_KEY}}", "");
    
    return result;
}

std::string HttpHandler::handleRequest(const std::string& request) {
    std::istringstream iss(request);
    std::string method, path;
    iss >> method >> path;

    std::string cookie = "";
    size_t cookiePos = request.find("Cookie: ");
    if (cookiePos != std::string::npos) {
        size_t cookieEnd = request.find("\r\n", cookiePos);
        cookie = request.substr(cookiePos + 8, cookieEnd - cookiePos - 8);
    }

    // GET /
    if (method == "GET" && path == "/") {
        std::string token = extractToken(cookie);
        User* user = shop.getUserByToken(token);
        
        if (user == nullptr) {
            std::string loginPage = readFile("templates/login.html");
            return "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + loginPage;
        } else {
            std::string mainTemplate = readFile("templates/main.html");
            std::string mainPage = renderTemplate(mainTemplate, *user);
            return "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + mainPage;
        }
    }

    // POST /login
    if (method == "POST" && path == "/login") {
       
        int initialCoins = 10; 
        size_t coinsHeaderPos = request.find("X-Coins: ");
        if (coinsHeaderPos != std::string::npos) {
            size_t coinsValueStart = coinsHeaderPos + 9;
            size_t coinsValueEnd = request.find("\r\n", coinsValueStart);
            if (coinsValueEnd != std::string::npos) {
                std::string coinsStr = request.substr(coinsValueStart, coinsValueEnd - coinsValueStart);
                try {
                    int parsedCoins = std::stoi(coinsStr);
                    if (parsedCoins >= 0) {
                        initialCoins = parsedCoins;
                    }
                } catch (...) {
                   
                }
            }
        }
        
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string body = request.substr(bodyPos + 4);
            size_t namePos = body.find("name=");
            if (namePos != std::string::npos) {
                std::string name = body.substr(namePos + 5);
                name = urlDecode(name);
                
                // Удаляем лишние символы в конце
                size_t endPos = name.find("&");
                if (endPos != std::string::npos) {
                    name = name.substr(0, endPos);
                }
                // Удаляем возможные \r\n в конце
                while (!name.empty() && (name.back() == '\r' || name.back() == '\n')) {
                    name.pop_back();
                }
                
                std::string token = shop.loginUser(name, initialCoins);
                
                // Если токен пустой, значит пользователь уже залогинен
                if (token.empty()) {
                    std::string errorPage = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><title>Ошибка</title></head><body><h1>Юзер занят</h1><p>Пользователь с таким именем уже залогинен.</p><a href=\"/\">Вернуться на главную</a></body></html>";
                    return "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + errorPage;
                }
                
                std::string response = "HTTP/1.1 302 Found\r\n";
                response += "Location: /\r\n";
                response += "Set-Cookie: token=" + token + "; Path=/\r\n";
                response += "\r\n";
                return response;
            }
        }
    }

    // POST /buySnowflake
    if (method == "POST" && path == "/buySnowflake") {
        std::string token = extractToken(cookie);
        if (token.empty() || shop.getUserByToken(token) == nullptr) {
            return "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
        }
        
        bool success = shop.buySnowflake(token);
        
        if (success) {
            std::string response = "HTTP/1.1 302 Found\r\n";
            response += "Location: /\r\n";
            response += "\r\n";
            return response;
        } else {
            std::string errorPage = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><title>Ошибка покупки</title></head><body><h1>Не удалось купить снежинку</h1><p>Недостаточно монет. Нужно 10 монет.</p><a href=\"/\">Вернуться на главную</a></body></html>";
            return "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + errorPage;
        }
    }

    // POST /buyTree
    if (method == "POST" && path == "/buyTree") {
        std::string token = extractToken(cookie);
        if (token.empty() || shop.getUserByToken(token) == nullptr) {
            return "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
        }
        
        bool success = shop.buyTree(token);
        
        if (success) {
            std::string response = "HTTP/1.1 302 Found\r\n";
            response += "Location: /\r\n";
            response += "\r\n";
            return response;
        } else {
            std::string errorPage = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><title>Ошибка покупки</title></head><body><h1>Не удалось купить елочку</h1><p>Недостаточно монет. Нужно 10 монет.</p><a href=\"/\">Вернуться на главную</a></body></html>";
            return "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + errorPage;
        }
    }

    // POST /buyGoldenTicket
    if (method == "POST" && path == "/buyGoldenTicket") {
        std::string token = extractToken(cookie);
        if (token.empty() || shop.getUserByToken(token) == nullptr) {
            return "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
        }
        
        bool success = shop.buyGoldenTicket(token);
        
        if (success) {
            std::string response = "HTTP/1.1 302 Found\r\n";
            response += "Location: /\r\n";
            response += "\r\n";
            return response;
        } else {
            std::string errorPage = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><title>Ошибка покупки</title></head><body><h1>Не удалось купить золотой билет</h1><p>Недостаточно ресурсов. Нужно 1 снежинка и 1 елочка.</p><a href=\"/\">Вернуться на главную</a></body></html>";
            return "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + errorPage;
        }
    }

    // POST /reset
    if (method == "POST" && path == "/reset") {
        std::string token = extractToken(cookie);
        if (token.empty() || shop.getUserByToken(token) == nullptr) {
            return "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
        }
        
        shop.resetUser(token);
        
        std::string response = "HTTP/1.1 302 Found\r\n";
        response += "Location: /\r\n";
        response += "\r\n";
        return response;
    }

    // POST /getSecretKey
    if (method == "POST" && path == "/getSecretKey") {
        std::string token = extractToken(cookie);
        if (token.empty() || shop.getUserByToken(token) == nullptr) {
            return "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
        }
        
        User* user = shop.getUserByToken(token);
        if (user->goldenTickets >= 1) {
            std::string secretKey = "it_is_not_a_real_flag";
            std::string keyPage = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><title>Секретный ключ</title><style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;}.container{background:white;padding:40px;border-radius:10px;box-shadow:0 10px 25px rgba(0,0,0,0.2);text-align:center;max-width:600px;}.key{background:#f5f5f5;padding:20px;border-radius:5px;font-family:'Courier New',monospace;font-size:18px;word-break:break-all;color:#333;border:2px solid #667eea;margin:20px 0;}.back-button{padding:12px 30px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;border-radius:5px;font-size:16px;font-weight:bold;cursor:pointer;text-decoration:none;display:inline-block;margin-top:20px;transition:transform 0.2s;}.back-button:hover{transform:translateY(-2px);}</style></head><body><div class=\"container\"><h1>🔐 Секретный ключ</h1><div class=\"key\">" + secretKey + "</div><a href=\"/\" class=\"back-button\">Вернуться на главную</a></div></body></html>";
            return "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + keyPage;
        } else {
            std::string errorPage = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><title>Ошибка</title><style>body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;}.container{background:white;padding:40px;border-radius:10px;box-shadow:0 10px 25px rgba(0,0,0,0.2);text-align:center;max-width:500px;}.back-button{padding:12px 30px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;border:none;border-radius:5px;font-size:16px;font-weight:bold;cursor:pointer;text-decoration:none;display:inline-block;margin-top:20px;transition:transform 0.2s;}.back-button:hover{transform:translateY(-2px);}</style></head><body><div class=\"container\"><h1>Ошибка</h1><p>У вас нет золотого билета для получения секретного ключа.</p><a href=\"/\" class=\"back-button\">Вернуться на главную</a></div></body></html>";
            return "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n" + errorPage;
        }
    }

    // POST /logout
    if (method == "POST" && path == "/logout") {
        std::string response = "HTTP/1.1 302 Found\r\n";
        response += "Location: /\r\n";
        response += "Set-Cookie: token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n";
        response += "\r\n";
        return response;
    }

    return "HTTP/1.1 404 Not Found\r\n\r\n";
}

