#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int client_socket;
    int target_socket;
} client_info;

void *client_handler(void *arg) {
    client_info *info = (client_info *)arg;
    int client_socket = info->client_socket;
    int target_socket = info->target_socket;
    char buffer[BUFFER_SIZE];

    while (1) {
        int read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            send(target_socket, buffer, strlen(buffer), 0);
        }
    }

    close(client_socket);
    return NULL;
}

void parse_arguments(int argc, char *argv[], int *port) {
    *port = DEFAULT_PORT;
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--port") == 0) {
            *port = atoi(argv[i + 1]);
        }
    }
}

int main(int argc, char *argv[]) {
    int server_fd, client_socket1, client_socket2, addr_len;
    struct sockaddr_in address;
    pthread_t thread1, thread2;
    int port;

    parse_arguments(argc, argv, &port);

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

    client_info info1 = {client_socket1, client_socket2};
    client_info info2 = {client_socket2, client_socket1};

    pthread_create(&thread1, NULL, client_handler, (void *)&info1);
    pthread_create(&thread2, NULL, client_handler, (void *)&info2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    close(server_fd);
    return 0;
}
