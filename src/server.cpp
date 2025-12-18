#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include "http_handler.h"
#include "../lib/shop.h"

class WebServer {
private:
    int port;
    int server_fd;
    Shop shop;
    HttpHandler handler;

public:
    WebServer(int p) : port(p), handler(shop) {}

    void start() {
        struct sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);

        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 3) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        std::cout << "Сервер запущен на порту " << port << std::endl;
        std::cout << "Откройте браузер и перейдите на http://localhost:" << port << std::endl;
        std::cout << "Многопоточный режим включен" << std::endl;

        while (true) {
            int new_socket;
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            std::thread client_thread([this, new_socket]() {
                std::string request;
                char buffer[4096] = {0};
                int bytes_read = read(new_socket, buffer, 4096);
                
                if (bytes_read > 0) {
                    request.append(buffer, bytes_read);
                    
                    // Если это POST запрос, читаем Content-Length и дочитываем тело
                    size_t contentLengthPos = request.find("Content-Length: ");
                    if (contentLengthPos != std::string::npos) {
                        size_t lengthStart = contentLengthPos + 16;
                        size_t lengthEnd = request.find("\r\n", lengthStart);
                        if (lengthEnd != std::string::npos) {
                            int contentLength = std::stoi(request.substr(lengthStart, lengthEnd - lengthStart));
                            size_t bodyStart = request.find("\r\n\r\n");
                            if (bodyStart != std::string::npos) {
                                bodyStart += 4;
                                int bodyRead = request.length() - bodyStart;
                                int remaining = contentLength - bodyRead;
                                
                                // Дочитываем оставшиеся байты тела
                                while (remaining > 0) {
                                    int toRead = remaining > 4096 ? 4096 : remaining;
                                    char bodyBuffer[4096] = {0};
                                    int bodyBytes = read(new_socket, bodyBuffer, toRead);
                                    if (bodyBytes <= 0) break;
                                    request.append(bodyBuffer, bodyBytes);
                                    remaining -= bodyBytes;
                                }
                            }
                        }
                    }
                    
                    std::string response = handler.handleRequest(request);
                    send(new_socket, response.c_str(), response.length(), 0);
                }
                
                close(new_socket);
            });
            
            client_thread.detach();
        }
    }
};

int main(int argc, char *argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    WebServer server(port);
    server.start();

    return 0;
}

