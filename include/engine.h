#ifndef ENGINE
#define ENGINE

#include <stdint.h>
#include <stddef.h>

struct prescal_engine{
    char *host;
    uint16_t port;
};

/* Prescal Engine function Definition */
struct prescal_engine *engine_init(char *host, uint16_t port);
void start(struct prescal_engine *engine);
void process_request(int fd);
void get_endpoint(const char *src, char *out, size_t size);

#endif