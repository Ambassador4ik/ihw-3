#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MONITOR_PORT 8081
#define BUFFER_SIZE 1024

void parse_arguments(int argc, char *argv[], char **server_ip, int *monitor_port) {
    *server_ip = "127.0.0.1";
    *monitor_port = MONITOR_PORT;

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--server-ip") == 0) {
            *server_ip = argv[i + 1];
        } else if (strcmp(argv[i], "--monitor-port") == 0) {
            *monitor_port = atoi(argv[i + 1]);
        }
    }
}

int main(int argc, char *argv[]) {
    int sock = 0, monitor_port;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char *server_ip;

    parse_arguments(argc, argv, &server_ip, &monitor_port);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(monitor_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    printf("Connected to monitor server at %s:%d\n", server_ip, monitor_port);

    while (1) {
        int read_size = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            printf("Monitor: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}
