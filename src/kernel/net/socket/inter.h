#pragma once

extern void sockop_set(int type, sockop_t *op);

extern sockop_t *sockop_get(int type);

extern int socket_makefd(socket_t *socket);
