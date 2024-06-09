#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
} client_info;

void *client_handler(void *arg) {
    client_info *info = (client_info *)arg;
    int client_socket = info->client_socket;
    struct sockaddr_in client_addr = info->client_addr;
    socklen_t addr_len = info->addr_len;
    char buffer[BUFFER_SIZE];

    while (1) {
        int read_size = recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr, addr_len);
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
    int server_fd;
    struct sockaddr_in address;
    pthread_t thread1, thread2;
    int port;

    parse_arguments(argc, argv, &port);

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

    printf("Server is listening on port %d\n", port);

    client_info info1 = {server_fd, address, sizeof(address)};
    client_info info2 = {server_fd, address, sizeof(address)};

    pthread_create(&thread1, NULL, client_handler, (void *)&info1);
    pthread_create(&thread2, NULL, client_handler, (void *)&info2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    close(server_fd);
    return 0;
}
