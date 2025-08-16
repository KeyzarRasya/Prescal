#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include "../include/http.h"
#include <string.h>

struct http_req *http_req_init() {
    struct http_req *http = 
        malloc(sizeof(struct http_req));
    
    if (!http) {
        perror("Failed to allocate http_request");
        exit(EXIT_FAILURE);
        return NULL;        
    }

    http->method = NULL;
    http->path = NULL;
    http->http_v = NULL;
    return http;
}

void extract_req(struct http_req *hreq, char *req) {
    char *token = strtok(req, " ");
    if (token) hreq->method = strdup(token);
    
    token = strtok(NULL, " ");
    if (token) hreq->path = strdup(token);

    token = strtok(NULL, " ");
    if (token) hreq->http_v = strdup(token);

    return hreq;
}

void req_to_string(struct http_req *hreq) {
    printf("Method %s\n Path %s\n Version %s\n", hreq->method, hreq->path, hreq->http_v);
}