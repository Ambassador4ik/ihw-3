#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_PORT 8080
#define MONITOR_PORT 8081
#define BUFFER_SIZE 1024

typedef struct {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int target_socket;
    pthread_mutex_t *monitor_mutex;
    int *monitor_sockets;
    int *monitor_count;
} client_info;

typedef struct {
    int monitor_socket;
    pthread_mutex_t *monitor_mutex;
    int *monitor_sockets;
    int *monitor_count;
} monitor_info;

void *client_handler(void *arg) {
    client_info *info = (client_info *)arg;
    int target_socket = info->target_socket;
    pthread_mutex_t *monitor_mutex = info->monitor_mutex;
    int *monitor_sockets = info->monitor_sockets;
    int *monitor_count = info->monitor_count;
    char buffer[BUFFER_SIZE];

    while (1) {
        int read_size = recvfrom(info->target_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&info->client_addr, &info->client_addr_len);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            sendto(target_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&info->client_addr, info->client_addr_len);

            pthread_mutex_lock(monitor_mutex);
            for (int i = 0; i < *monitor_count; i++) {
                sendto(monitor_sockets[i], buffer, strlen(buffer), 0, (struct sockaddr *)&info->client_addr, info->client_addr_len);
            }
            pthread_mutex_unlock(monitor_mutex);
        }
    }

    close(target_socket);
    return NULL;
}

void *monitor_handler(void *arg) {
    monitor_info *info = (monitor_info *)arg;
    int monitor_socket = info->monitor_socket;
    pthread_mutex_t *monitor_mutex = info->monitor_mutex;
    int *monitor_sockets = info->monitor_sockets;
    int *monitor_count = info->monitor_count;

    pthread_mutex_lock(monitor_mutex);
    monitor_sockets[*monitor_count] = monitor_socket;
    (*monitor_count)++;
    pthread_mutex_unlock(monitor_mutex);

    // Ожидание завершения работы клиента
    char buffer[BUFFER_SIZE];
    while (recvfrom(monitor_socket, buffer, BUFFER_SIZE, 0, NULL, NULL) > 0);

    pthread_mutex_lock(monitor_mutex);
    for (int i = 0; i < *monitor_count; i++) {
        if (monitor_sockets[i] == monitor_socket) {
            for (int j = i; j < *monitor_count - 1; j++) {
                monitor_sockets[j] = monitor_sockets[j + 1];
            }
            (*monitor_count)--;
            break;
        }
    }
    pthread_mutex_unlock(monitor_mutex);

    close(monitor_socket);
    return NULL;
}

void parse_arguments(int argc, char *argv[], int *port, int *monitor_port) {
    *port = DEFAULT_PORT;
    *monitor_port = MONITOR_PORT;
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--port") == 0) {
            *port = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--monitor-port") == 0) {
            *monitor_port = atoi(argv[i + 1]);
        }
    }
}

int main(int argc, char *argv[]) {
    int server_fd, monitor_fd, addr_len;
    struct sockaddr_in address, monitor_address;
    pthread_t client_thread, monitor_thread;
    int port, monitor_port;

    int monitor_sockets[FD_SETSIZE];
    int monitor_count = 0;
    pthread_mutex_t monitor_mutex = PTHREAD_MUTEX_INITIALIZER;

    parse_arguments(argc, argv, &port, &monitor_port);

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if ((monitor_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    monitor_address.sin_family = AF_INET;
    monitor_address.sin_addr.s_addr = INADDR_ANY;
    monitor_address.sin_port = htons(monitor_port);

    if (bind(monitor_fd, (struct sockaddr *)&monitor_address, sizeof(monitor_address)) < 0) {
        perror("Bind failed");
        close(monitor_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);
    printf("Waiting for monitor clients on port %d\n", monitor_port);

    addr_len = sizeof(address);

    while (1) {
        client_info *client_info1 = malloc(sizeof(client_info));
        client_info1->target_socket = server_fd;
        client_info1->monitor_mutex = &monitor_mutex;
        client_info1->monitor_sockets = monitor_sockets;
        client_info1->monitor_count = &monitor_count;

        recvfrom(server_fd, NULL, 0, 0, (struct sockaddr *)&client_info1->client_addr, &client_info1->client_addr_len);
        printf("Client connected\n");

        pthread_create(&client_thread, NULL, client_handler, (void *)client_info1);

        monitor_info *info = malloc(sizeof(monitor_info));
        info->monitor_socket = monitor_fd;
        info->monitor_mutex = &monitor_mutex;
        info->monitor_sockets = monitor_sockets;
        info->monitor_count = &monitor_count;

        pthread_create(&monitor_thread, NULL, monitor_handler, (void *)info);
        pthread_detach(monitor_thread);
    }

    pthread_join(client_thread, NULL);

    close(server_fd);
    close(monitor_fd);
    return 0;
}
