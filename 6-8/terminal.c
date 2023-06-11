#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Использование: %s <адрес сервера> <порт сервера>\n", argv[0]);
        return 1;
    }

    const char *server_address = argv[1];
    const int server_port = atoi(argv[2]);
    int client_fd, valread;
    struct sockaddr_in server_addr;
    char *message = "-2";
    char buffer[BUFFER_SIZE] = {0};

    // Создание сокета
    if ((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\nСоздание сокета не удалось\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Преобразование IP-адреса из текстового формата в бинарный
    if (inet_pton(AF_INET, server_address, &server_addr.sin_addr) <= 0) {
        printf("\nОшибка в преобразовании IP-адреса\n");
        return -1;
    }

    sendto(client_fd, message, strlen(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Сообщение отправлено серверу\n");

    while (1) {
        // Получение ответа от сервера
        char buffer[BUFFER_SIZE];
        ssize_t valread = recvfrom(client_fd, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);

        printf("Получен ответ от сервера: %s\n", buffer);

        memset(buffer, 0, BUFFER_SIZE); // Очистка буфера

        // Ожидание перед отправкой следующего сообщения
        sleep(1);
    }
}