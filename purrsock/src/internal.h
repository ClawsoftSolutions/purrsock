// Copyright (c) 2024 ClawsoftSolutions. All rights reserved.
//
// This software is licensed under the MIT License.
// See LICENSE file for more information.
//



#ifndef   INTERNAL_H
#define   INTERNAL_H

#include "purrsock/purrsock.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

typedef struct {
  ps_protocol_t protocol;
  void *data;
  struct sockaddr_storage addr_storage;
} _purrsock_socket_t;

// Definitions

const char* get_platform();
void logWSAError(int error);

bool _purrsock_init();
void _purrsock_cleanup();

ps_result_t _purrsock_create_socket(_purrsock_socket_t *socket);
ps_result_t _purrsock_create_socket_from_addr(_purrsock_socket_t *socket, const char *ip, ps_port_t port);
void _purrsock_destroy_socket(_purrsock_socket_t *socket);

ps_result_t _purrsock_bind_socket(_purrsock_socket_t *socket, const char *ip, ps_port_t port);
ps_result_t _purrsock_listen_socket(_purrsock_socket_t *socket);
ps_result_t _purrsock_accept_socket(_purrsock_socket_t *socket, _purrsock_socket_t **client);

ps_result_t _purrsock_connect_socket(_purrsock_socket_t *socket, const char *ip, ps_port_t port);

ps_result_t _purrsock_read_socket_packet(_purrsock_socket_t *socket, ps_packet_t *packet, _purrsock_socket_t **from);
ps_result_t _purrsock_send_socket_packet(_purrsock_socket_t *socket, ps_packet_t packet, _purrsock_socket_t *to);

#endif // INTERNAL_H