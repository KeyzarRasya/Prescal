#ifndef HTTP
#define HTTP

struct http_req {
    char *method;
    char *path;
    char *http_v;
};

/* http req function definitions */
struct http_req *http_req_init();
void extract_req(struct http_req *hreq, char *req);
void req_to_string(struct http_req *req);


#endif