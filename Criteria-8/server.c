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
    int client_socket;
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
    int client_socket = info->client_socket;
    int target_socket = info->target_socket;
    pthread_mutex_t *monitor_mutex = info->monitor_mutex;
    int *monitor_sockets = info->monitor_sockets;
    int *monitor_count = info->monitor_count;
    char buffer[BUFFER_SIZE];

    while (1) {
        int read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            send(target_socket, buffer, strlen(buffer), 0);

            pthread_mutex_lock(monitor_mutex);
            for (int i = 0; i < *monitor_count; i++) {
                send(monitor_sockets[i], buffer, strlen(buffer), 0);
            }
            pthread_mutex_unlock(monitor_mutex);
        }
    }

    close(client_socket);
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
    while (recv(monitor_socket, buffer, BUFFER_SIZE, 0) > 0);

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
    int server_fd, monitor_fd, client_socket1, client_socket2, addr_len;
    struct sockaddr_in address;
    pthread_t thread1, thread2, monitor_thread;
    int port, monitor_port;

    int monitor_sockets[FD_SETSIZE];
    int monitor_count = 0;
    pthread_mutex_t monitor_mutex = PTHREAD_MUTEX_INITIALIZER;

    parse_arguments(argc, argv, &port, &monitor_port);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
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

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    addr_len = sizeof(address);
    client_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
    if (client_socket1 < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client 1 connected\n");

    client_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
    if (client_socket2 < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client 2 connected\n");

    if ((monitor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_port = htons(monitor_port);

    if (bind(monitor_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(monitor_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(monitor_fd, 3) < 0) {
        perror("Listen failed");
        close(monitor_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for monitor clients on port %d\n", monitor_port);

    client_info info1 = {client_socket1, client_socket2, &monitor_mutex, monitor_sockets, &monitor_count};
    client_info info2 = {client_socket2, client_socket1, &monitor_mutex, monitor_sockets, &monitor_count};

    pthread_create(&thread1, NULL, client_handler, (void *)&info1);
    pthread_create(&thread2, NULL, client_handler, (void *)&info2);

    while (1) {
        int monitor_socket = accept(monitor_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
        if (monitor_socket < 0) {
            perror("Accept failed");
            close(monitor_fd);
            exit(EXIT_FAILURE);
        }

        printf("Monitor client connected\n");

        monitor_info *info = malloc(sizeof(monitor_info));
        info->monitor_socket = monitor_socket;
        info->monitor_mutex = &monitor_mutex;
        info->monitor_sockets = monitor_sockets;
        info->monitor_count = &monitor_count;

        pthread_create(&monitor_thread, NULL, monitor_handler, (void *)info);
        pthread_detach(monitor_thread);
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    close(server_fd);
    close(monitor_fd);
    return 0;
}
