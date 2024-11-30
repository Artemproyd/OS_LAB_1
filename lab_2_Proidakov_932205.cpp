#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <sys/select.h>

// Глобальная переменная для обработки сигнала
volatile sig_atomic_t signal_received = 0;

// Обработчик сигнала
void signal_handler(int signal) {
    signal_received = signal;
}

int main() {
    const int PORT = 1111; // Порт для прослушивания
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    std::vector<int> client_sockets;

    // 1. Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Установка параметров сокета
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET; // Указываем, что адрес IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Любой IP-адрес на этом хосте
    address.sin_port = htons(PORT); // Порт 1111 в сетевом порядке байтов
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 2. Регистрация обработчика сигнала
   struct sigaction sa;
   sa.sa_handler = signal_handler;  // Указываем функцию, которая будет вызываться при сигнале
   sa.sa_flags = 0;                 // Флаги (по умолчанию — никаких дополнительных настроек)
   sigemptyset(&sa.sa_mask);        // Очищаем маску блокируемых сигналов
   if (sigaction(SIGHUP, &sa, nullptr) == -1) {
        perror("Sigaction failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Блокирование сигнала
    sigset_t block_mask, old_mask;
    sigemptyset(&block_mask);               // Очищаем набор сигналов
    sigaddset(&block_mask, SIGHUP);         // Добавляем сигнал SIGHUP в блокируемый набор
        if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1) {
        perror("Sigprocmask failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running on port " << PORT << std::endl;

    // 4. Основной цикл
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);

        // Добавляем серверный сокет в список
        FD_SET(server_fd, &readfds);
        int max_fd = server_fd;

        // Добавляем клиентские сокеты в список
        for (int client_fd : client_sockets) {
            FD_SET(client_fd, &readfds);
            if (client_fd > max_fd) max_fd = client_fd;
        }

        // 5. Вызов pselect()
        int ready_count;
        ready_count = pselect(max_fd + 1, &readfds, nullptr, nullptr, nullptr, &old_mask);

        // Обработка ошибок
        if (ready_count == -1) {
            if (errno == EINTR && signal_received == SIGHUP) {
                std::cout << "SIGHUP received, handling signal..." << std::endl;
                signal_received = 0; // Сброс флага
                continue; // Продолжаем работу
            }
            perror("Pselect failed");
            break;
        }

        // Новый запрос на соединение
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
            if (new_socket < 0) {
                perror("Accept failed");
                continue;
            }
            client_sockets.push_back(new_socket);
            std::cout << "New connection accepted" << std::endl;
        }

        // Чтение данных с клиентских сокетов
        for (auto it = client_sockets.begin(); it != client_sockets.end();) {
            int client_fd = *it;
            if (FD_ISSET(client_fd, &readfds)) {
                char buffer[1024] = {0};
                int bytes_read = read(client_fd, buffer, sizeof(buffer));
                if (bytes_read <= 0) {
                    // Закрытие соединения
                    std::cout << "Client disconnected" << std::endl;
                    close(client_fd);
                    it = client_sockets.erase(it);
                } else {
                    std::cout << "Received: " << buffer << std::endl;
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    // Завершение работы
    close(server_fd);
    for (int client_fd : client_sockets) {
        close(client_fd);
    }
    return 0;
}

