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
    struct sockaddr_in client_addr;
    struct sockaddr_in monitor_addr;
    socklen_t addr_len;
} client_info;

void *client_handler(void *arg) {
    client_info *info = (client_info *)arg;
    int client_socket = info->client_socket;
    struct sockaddr_in client_addr = info->client_addr;
    struct sockaddr_in monitor_addr = info->monitor_addr;
    socklen_t addr_len = info->addr_len;
    char buffer[BUFFER_SIZE];

    while (1) {
        int read_size = recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr, addr_len);
            if (monitor_addr.sin_port != 0) {
                sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&monitor_addr, addr_len);
            }
        }
    }

    close(client_socket);
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
    int server_fd, monitor_fd, port, monitor_port;
    struct sockaddr_in server_addr, monitor_addr, client_addr;
    socklen_t addr_len;
    pthread_t thread1, thread2;

    parse_arguments(argc, argv, &port, &monitor_port);

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    memset(&monitor_addr, 0, sizeof(monitor_addr));
    monitor_addr.sin_family = AF_INET;
    monitor_addr.sin_port = htons(monitor_port);

    addr_len = sizeof(client_addr);
    recvfrom(server_fd, NULL, 0, 0, (struct sockaddr *)&client_addr, &addr_len);

    client_info info1 = {server_fd, client_addr, monitor_addr, addr_len};
    pthread_create(&thread1, NULL, client_handler, (void *)&info1);

    recvfrom(server_fd, NULL, 0, 0, (struct sockaddr *)&client_addr, &addr_len);
    client_info info2 = {server_fd, client_addr, monitor_addr, addr_len};
    pthread_create(&thread2, NULL, client_handler, (void *)&info2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    close(server_fd);
    return 0;
}
