#define _POSIX_C_SOURCE 200809L
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

    const char *response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "Hello, World!";

    char buff[1024];

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
    char req[1024];
    char *res = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "Hello, World!";
    int n = recv(fd, req, sizeof(req), 0);
    if (n < 0) {
        perror("Failed to receive request");
        return;
    }

    req[n] = '\0';
    send(fd, res, strlen(res), 0);
    printf("%s\n", req);
    char endpoint[256];
    get_endpoint(req, endpoint, sizeof(endpoint));
    struct http_req *hreq = http_req_init();
    extract_req(hreq, strdup(endpoint));
    req_to_string(hreq);
    free(hreq);
}

void get_endpoint(const char *src, char *out, size_t size) {
    int i = 0;
    while (src[i] != '\n' && i < size - 1) {
        out[i] = src[i];
        i++;
    }
    out[i] = '\0';
}

