#ifndef ENGINE
#define ENGINE

#include <stdint.h>

struct prescal_engine{
    char *host;
    uint16_t port;
};

struct prescal_engine *engine_init(char *host, uint16_t port);
void start(struct prescal_engine *engine);

#endif