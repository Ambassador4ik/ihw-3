#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int x;
    int y;
    int value;
} Target;

typedef struct {
    int sock;
    int K;
    int h;
    int w;
    int sum_value;
    struct sockaddr_in server_addr;
    Target *targets;
} client_info;

void parse_arguments(int argc, char *argv[], int *K, int *h, int *w, char **server_ip, int *server_port) {
    *server_ip = DEFAULT_SERVER_IP;
    *server_port = DEFAULT_SERVER_PORT;

    if (argc < 7) {
        printf("Usage: ./client --targets <K> --height <h> --width <w> [--server-ip <ip>] [--server-port <port>]\n");
        exit(1);
    }

    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--targets") == 0) {
            *K = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--height") == 0) {
            *h = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--width") == 0) {
            *w = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--server-ip") == 0) {
            *server_ip = argv[i + 1];
        } else if (strcmp(argv[i], "--server-port") == 0) {
            *server_port = atoi(argv[i + 1]);
        } else {
            printf("Usage: ./client --targets <K> --height <h> --width <w> [--server-ip <ip>] [--server-port <port>]\n");
            exit(1);
        }
    }
}

void initialize_targets(Target *targets, int K, int h, int w, int *sum_value) {
    srand(time(NULL));
    *sum_value = 0;
    for (int i = 0; i < K; i++) {
        targets[i].x = rand() % w;
        targets[i].y = rand() % h;
        targets[i].value = (rand() % 9) + 1;
        *sum_value += targets[i].value;
    }
}

void print_field(Target *targets, int K, int h, int w, int latest_x, int latest_y, int latest_hit, int remaining_targets, int remaining_sum_value) {
    int field[h][w];
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            field[i][j] = 0;
        }
    }
    for (int i = 0; i < K; i++) {
        field[targets[i].y][targets[i].x] = targets[i].value;
    }

    printf("\033[H\033[J");  // Очистка экрана
    printf(">> Battle Simulation <<\n");
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (field[i][j] > 0) {
                printf("\033[1;31m[%d]\033[0m ", field[i][j]);
            } else {
                printf("[%d] ", field[i][j]);
            }
        }
        printf("\n");
    }
    printf("Latest shot at (%d, %d) was %s.\n", latest_x, latest_y, latest_hit ? "successful" : "unsuccessful");
    printf("%d Targets Alive | %d Total Value\n", remaining_targets, remaining_sum_value);
}

void *receive_shots(void *arg) {
    client_info *info = (client_info *)arg;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    while (info->K > 0) {
        int read_size = recvfrom(info->sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from_addr, &addr_len);
        if (read_size > 0) {
            buffer[read_size] = '\0';

            // Парсинг координат от сервера
            int x, y;
            sscanf(buffer, "x=%d y=%d", &x, &y);

            // Проверка поражения цели
            int target_hit = 0;
            for (int i = 0; i < info->K; i++) {
                if (info->targets[i].x == x && info->targets[i].y == y) {
                    target_hit = 1;
                    info->sum_value -= info->targets[i].value;
                    info->targets[i] = info->targets[info->K - 1]; // Заменяем текущую цель последней
                    info->K--; // Уменьшаем количество целей
                    break;
                }
            }

            // Обновление состояния
            int latest_x = x;
            int latest_y = y;
            int latest_hit = target_hit;

            // Отправка данных обратно на сервер
            sprintf(buffer, "target_hit=%d remaining_targets=%d remaining_sum_value=%d", target_hit, info->K, info->sum_value);
            sendto(info->sock, buffer, strlen(buffer), 0, (struct sockaddr *)&info->server_addr, addr_len);

            // Обновление поля
            print_field(info->targets, info->K, info->h, info->w, latest_x, latest_y, latest_hit, info->K, info->sum_value);

            if (info->K == 0) {
                printf("No targets left. Simulation finished.\n");
                break;
            }
        }
    }

    return NULL;
}

void *send_shots(void *arg) {
    client_info *info = (client_info *)arg;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(info->server_addr);

    while (info->K > 0) {
        // Генерация случайного интервала T
        int T = (rand() % 5) + 1;
        sleep(T);

        // Генерация случайных координат
        int x = rand() % info->w;
        int y = rand() % info->h;
        sprintf(buffer, "x=%d y=%d", x, y);
        sendto(info->sock, buffer, strlen(buffer), 0, (struct sockaddr *)&info->server_addr, addr_len);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int K, h, w, sum_value;
    char *server_ip;
    int server_port;

    parse_arguments(argc, argv, &K, &h, &w, &server_ip, &server_port);

    Target targets[K];
    initialize_targets(targets, K, h, w, &sum_value);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Отправка данных о поле и мишенях на сервер
    sprintf(buffer, "height=%d width=%d targets=%d sum_value=%d", h, w, K, sum_value);
    sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // Инициализация данных для обновления состояния
    client_info info = {sock, K, h, w, sum_value, serv_addr, targets};
    pthread_t recv_thread, send_thread;

    pthread_create(&recv_thread, NULL, receive_shots, (void *)&info);
    pthread_create(&send_thread, NULL, send_shots, (void *)&info);

    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sock);
    return 0;
}
