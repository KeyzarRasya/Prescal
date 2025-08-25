#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <stddef.h>
#include "../include/http.h"
#include "../include/engine.h"
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "timer.h"
#include "metrics.h"

#define DEBUG 0                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
atomic_int rps_a = ATOMIC_VAR_INIT(0);
double cpu_t0;

void *request_per_second() {
    FILE *fptr = fopen("/tmp/data.txt", "a");
    if (!fptr) {
        perror("file pointer");
        return NULL;
    }

    char metrics_res[MAX_METRICS_RESPONSE];
    if (request_metrics(3000, metrics_res, sizeof(metrics_res)) != 0) {
        perror("CPU t0 initial");
        return NULL;
    }
    cpu_t0 = get_value(metrics_res, CPU_SECOND);

    while(1) {
        sleep_ms(1000);
        struct metrics *metrics = init_metrics();
        if (request_metrics(3000, metrics_res, sizeof(metrics_res)) != 0) {
            perror("metrics request");
            break;
        }
        store_metrics(metrics, metrics_res);

        double current_cpu = metrics->cpu_usage;
        calculate_metrics(metrics, cpu_t0);

        int rps = atomic_exchange(&rps_a, 0);
        write_metrics(fptr, metrics, rps);
        cpu_t0 = current_cpu;
        rps = 0;
        free(metrics);
        memset(metrics_res, 0, sizeof(metrics_res));
    }
    fclose(fptr);
    return NULL;
}

/* Static Function */
static int handle_request(int fd, struct http_req *hreq) {
    char request[MAX_BUFF_SIZE];
    int n = recv(fd, request, sizeof(request), 0);
    if (n < 0) {
        perror("Failed to receive request");
        return -1;
    }
    request[n] = '\0';
    convert_request(hreq, request, sizeof(request));
    return 0;
}

static void handle_response(int fd, struct http_req *hreq) {
    char response[MAX_BUFF_SIZE];
    if (forwards(hreq->raw, response, sizeof(response)) != 0) {
        snprintf(response, sizeof(response), "%s", ON_SOCK_ERR);
    }
    send(fd, response, sizeof(response), 0);
}

void log_elapsed_time(struct timespec start, struct timespec end) {
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Elapsed time: %0.6f\n", elapsed);
}

static int connect_to_server(int fd) {
    int port = 3000 + (rand() % 3);
    struct sockaddr_in destiny = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };
    inet_pton(AF_INET, "127.0.0.1", &destiny.sin_addr);

    if (connect(fd, (struct sockaddr*)&destiny, sizeof(destiny)) < 0) {
        perror("Failed while connecting to server");
        return -1;
    }
    return 0;
}

void init_listener(int fd, struct prescal_engine *engine) {
    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(engine->port),
        .sin_addr = INADDR_ANY
    };
    
    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("socket option");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (bind(fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("bind");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (listen(fd, SOMAXCONN) < 0) {
        perror("listen");
        close(fd);
        exit(EXIT_FAILURE);
    }

}

void handle_connections(int fd) {
    struct sockaddr_in client;
    socklen_t client_size = sizeof(client);
    while (1) {
        int conn = accept(fd, (struct sockaddr*)&client, &client_size);
        if (conn < 0) {
            perror("accept");
            continue;
        }
        process_request(conn);
        close(conn);
    }
}
/* END */


struct prescal_engine *engine_init(char *host, uint16_t port) {
    struct prescal_engine *engine = 
        malloc(sizeof(struct prescal_engine));

    if (!engine) {
        perror("Failed to allocate engine");
        return NULL;
    }
    engine->host = host;
    engine->port = port;
    engine->config = config_init();

    read_config(engine->config);
    return engine;
}

void start(struct prescal_engine *engine) {
    pthread_t rps_reset_thread;
    pthread_create(&rps_reset_thread, NULL, request_per_second, NULL);
    
    int fd;
    struct sockaddr_in client_addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Failed to create socket");
        return;
    }

    init_listener(fd, engine);
    handle_connections(fd);

    close(fd);
    return;
}

void process_request(int fd) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    struct http_req *hreq = http_req_init();

    if (handle_request(fd, hreq) != 0) {
        free(hreq);
        return;
    }
    atomic_fetch_add(&rps_a, 1);

    handle_response(fd, hreq);
    clock_gettime(CLOCK_MONOTONIC, &end);

    #if DEBUG
        log_elapsed_time(start, end);
    #endif
    
    free(hreq);
}

int forwards(const char *request, char *http_response, size_t size) {
    int fd;
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        close(fd);
        return -1;
    }

    if (connect_to_server(fd) != 0) {
        close(fd);
        return -1;
    }

    send(fd, request, strlen(request), 0);
    recv(fd, http_response, size, 0);

    close(fd);
    return 0;
}

void destroy_engine(struct prescal_engine *engine) {
    free(engine->config->forwards);
    free(engine->config);
    free(engine);
}