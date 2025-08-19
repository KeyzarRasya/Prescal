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

char *res = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "Hello, World!";

struct prescal_engine *engine_init(char *host, uint16_t port) {
    struct prescal_engine *engine = 
        malloc(sizeof(struct prescal_engine));

    if (!engine) {
        perror("Failed to allocate engine");
        return NULL;
    }

    engine->host = host;
    engine->port = port;
    return engine;
}

void start(struct prescal_engine *engine) {

    struct sockaddr_in server_addr, client_addr;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Failed to create socket");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(engine->port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        close(fd);
        perror("Failed to set socket options");
        exit(EXIT_FAILURE);
    }

    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(fd);
        perror("Failed to bind address");
        return;
    }

    if (listen(fd, 5) == -1) {
        perror("failed to listen");
        return;
    }
    socklen_t client_len = sizeof(struct sockaddr_in);

    printf("Listening...\n");
    while(1) {
        int c = accept(fd, (struct sockaddr*)&client_addr, &client_len);
        process_request(c);
        close(c);
    }

    close(fd);
    printf("Success");
    return;
}

void process_request(int fd) {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    char req[1024];
    char response[1024];
    struct http_req *hreq = http_req_init();

    int n = recv(fd, req, sizeof(req), 0);
    if (n < 0) {
        perror("Failed to receive request");
        free(hreq);
        return;
    }
    req[n] = '\0';
    convert_request(hreq, req, sizeof(req));
    forwards(req, response, sizeof(response));
    req_to_string(hreq);

    send(fd, response, strlen(response), 0);


    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Elapsed time: %.6f seconds\n", elapsed);
    
    free(hreq);
}

void forwards(const char *request, char *http_response, size_t size) {
    int fd;
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(3000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        http_response = strdup(ON_SOCK_ERR);
        close(fd);
        return;
    }

    if (connect(fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        http_response = strdup(ON_SOCK_ERR);
        close(fd);
        return;
    }

    send(fd, request, strlen(request), 0);
    recv(fd, http_response, size, 0);

    close(fd);
}