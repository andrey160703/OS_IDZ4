#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

pthread_mutex_t mutex_log;

int length, width, treasure = -1;

#define MAX_MESSAGES 10000
#define MAX_MESSAGE_LENGTH 100

char messages[MAX_MESSAGES][MAX_MESSAGE_LENGTH];

struct ThreadData {
    int server_fd;
    struct sockaddr_in client_addr;
    socklen_t addrlen;
    pthread_mutex_t mutex_log;
    int* message_count;
    // Дополнительные поля данных, если нужно
};

int message_count = 0, current_index = 0;
// Добавление сообщения в массив
void add_message(const char* message) {
    if (message_count < MAX_MESSAGES) {
        strncpy(messages[message_count], message, MAX_MESSAGE_LENGTH - 1);
        messages[message_count][MAX_MESSAGE_LENGTH - 1] = '\0';
        message_count++;
        if (message_count % 20 == 0 && current_index > 0) {
            current_index += 10;
        }
    }
}

// Получение сообщения по индексу
const char* get_message(int index) {
    if (index >= 0 && index < message_count) {
        return messages[index];
    }
    return NULL;
}

// Очистка массива сообщений
void clear_messages() {
    message_count = 0;
}

void* threadFunction(void* arg) {
    // Sleep for 5 seconds
    sleep(5);
    printf("Server work ended.\n");
    exit(0);
    pthread_exit(NULL);
}

void* terminalProcessing(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    int server_fd = data->server_fd;
    struct sockaddr_in client_addr = data->client_addr;
    socklen_t addrlen = data->addrlen;
    pthread_mutex_t mutex_log = data->mutex_log;
    int* message_count = data->message_count;

    int ind = current_index;
    while (1) {
        pthread_mutex_lock(&mutex_log);
        int end = *message_count;
        pthread_mutex_unlock(&mutex_log);
        for (int i = ind; i < end; i++) {
            sendto(server_fd, messages[i], strlen(messages[i]), 0, (struct sockaddr *)&client_addr, addrlen);
        }
        if (ind == end) {
            sendto(server_fd, "----------------\n", strlen("----------------\n"), 0, (struct sockaddr *)&client_addr, addrlen);
        }
        ind = *message_count;
        sleep(1);
    }

    return NULL;
}


void IncreaseCoordinates(int *x, int *y) {
    (*x)++;
    if (*x == length) {
        *x = 0;
        (*y)++;
    }
    if (*y == width) {
        *y = 0;
    }
}

int main(int argc, char *argv[]) {
    pthread_mutex_init(&mutex_log, NULL);
    int flag = 0;

    pthread_t thread;
    if (argc != 3) {
        printf("Использование: %s <адрес> <порт>\n", argv[0]);
        return -1;
    }

    srand((int) time(0));
    length = rand() % 10 + 2, width = rand() % 10 + 2;

    int server_fd, valread;
    struct sockaddr_in server_addr, client_addr;
    int addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};
    char *response = "Привет! Это сервер.";

    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Не удалось создать сокет");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Привязка сокета к адресу и порту
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Привязка сокета не удалась");
        exit(EXIT_FAILURE);
    }

    printf("Ожидание сообщений от клиентов...\n");

    int x = 0, y = 0;

    int number = 0;

    while (1) {
        // Прием сообщения от клиента
        valread = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
        if (valread > 0) {
            buffer[valread] = '\0'; // Добавляем завершающий нулевой символ
            number = atoi(buffer); // Преобразование строки в целое число
            char numberString[100];
            sprintf(numberString, "Получено число от клиента: %d\n", number);
            add_message(numberString);
            printf("Получено число от клиента: %d\n", number);
        } else {
            printf("Ошибка при получении сообщения от клиента\n");
        }

        char message[100];
        snprintf(message, sizeof(message), "Флинт послал искать в {%d:%d}\n", x, y);

        if (number > 0) {
            treasure = number;
        }

        if (treasure == -1) { /// клиент - команда
            IncreaseCoordinates(&x, &y);
            add_message(message);
            sendto(server_fd, message, strlen(message), 0, (struct sockaddr *)&client_addr, addrlen);
            printf("Отправлен ответ клиенту\n");
        }
        if (number == -2) { // клиент - терминал
            struct ThreadData* data = malloc(sizeof(struct ThreadData));
            data->server_fd = server_fd;
            data->client_addr = client_addr;
            data->addrlen = addrlen;
            data->mutex_log = mutex_log;
            data->message_count = &message_count;
            pthread_create(&thread, NULL, terminalProcessing, (void*)data);
            pthread_detach(thread);
        }
        if (treasure > 0) { /// команда нашла клад
            snprintf(message, sizeof(message), "-1");
            sendto(server_fd, message, strlen(message), 0, (struct sockaddr *)&client_addr, addrlen);
            printf("Отправлен ответ клиенту\n");
            if (flag == 0) {
                flag = 1;
                pthread_create(&thread, NULL, threadFunction, NULL);
            }
        }
        memset(buffer, 0, BUFFER_SIZE); // Очистка буфера
    }
    pthread_mutex_destroy(&mutex_log);
    return 0;
}
