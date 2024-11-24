// Copyright (c) 2024 ClawsoftSolutions. All rights reserved.
//
// This software is licensed under the MIT License.
// See LICENSE file for more information.
//

#ifdef __linux__

#include "internal.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static bool is_initialized = false;

bool _purrsock_init() {
  if (is_initialized) return true;
  is_initialized = true;
  const char* platform = get_platform();
  printf("Running on: %s\n", platform);
  return true;
}

void _purrsock_cleanup() {
  is_initialized = false;
}

ps_result_t _purrsock_create_socket(_purrsock_socket_t *socket) {
  assert(socket);

  int domain = (socket->protocol == PS_PROTOCOL_TCP) ? AF_INET : AF_INET6;
  int type = (socket->protocol == PS_PROTOCOL_TCP) ? SOCK_STREAM : SOCK_DGRAM;
  int protocol = 0;

  int sockfd = socket(domain, type, protocol);
  if (sockfd < 0) {
    return PS_ERROR_INTERNAL; 
  }

  socket->sockfd = sockfd;
  return PS_SUCCESS;
}

ps_result_t _purrsock_create_socket_from_addr(_purrsock_socket_t *socket, const char *ip, ps_port_t port) {
  assert(socket);

  ps_result_t result = _purrsock_create_socket(socket);
  if (result != PS_SUCCESS) {
    return result;
  }

  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(port);

  if (inet_pton(AF_INET6, ip, &addr.sin6_addr) <= 0) {
    if (inet_pton(AF_INET, ip, &addr.sin6_addr) <= 0) {
      return PS_ERROR_ADDRINUSE; 
    } else {
      addr.sin6_family = AF_INET;
    }
  }

  if (bind(socket->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return PS_ERROR_ADDRINUSE;  
  }

  return PS_SUCCESS;
}

void _purrsock_destroy_socket(_purrsock_socket_t *socket) {
  assert(socket);
  close(socket->sockfd);
}

ps_result_t _purrsock_bind_socket(_purrsock_socket_t *socket, const char *ip, ps_port_t port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);

  if (bind(socket->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return PS_ERROR_ADDRINUSE; 
  }

  return PS_SUCCESS;
}

ps_result_t _purrsock_listen_socket(_purrsock_socket_t *socket) {
  if (listen(socket->sockfd, 10) < 0) {
    return PS_ERROR_INTERNAL;  
  }
  return PS_SUCCESS;
}

ps_result_t _purrsock_accept_socket(_purrsock_socket_t *socket, _purrsock_socket_t **client) {
  assert(client);
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  int client_sock = accept(socket->sockfd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_sock < 0) {
    return PS_ERROR_CONNRESET; 
  }

  _purrsock_socket_t *new_client = (_purrsock_socket_t *)malloc(sizeof(*new_client));
  if (!new_client) {
    return PS_ERROR_INTERNAL; 
  }

  new_client->sockfd = client_sock;
  new_client->protocol = socket->protocol;
  *client = new_client;

  return PS_SUCCESS;
}

ps_result_t _purrsock_connect_socket(_purrsock_socket_t *socket, const char *ip, ps_port_t port) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);

  if (connect(socket->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    return PS_ERROR_CONNREFUSED; 
  }

  return PS_SUCCESS;
}

ps_result_t _purrsock_read_socket_packet(_purrsock_socket_t *socket, ps_packet_t *packet, _purrsock_socket_t **from) {
  assert(socket && packet);

  char buffer[1024];
  ssize_t len = recv(socket->sockfd, buffer, sizeof(buffer), 0);
  if (len < 0) {
    return PS_ERROR_INTERNAL;
  }

  packet->size = len;
  packet->buf = (char *)malloc(len);
  if (!packet->buf) {
    return PS_ERROR_INTERNAL; 
  }
  memcpy(packet->buf, buffer, len);

  if (from) {
    *from = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memcpy(*from, &addr, sizeof(struct sockaddr_in));
  }


  return PS_SUCCESS;
}

ps_result_t _purrsock_send_socket_packet(_purrsock_socket_t *socket, ps_packet_t packet, _purrsock_socket_t *to) {
  assert(socket && to && packet.buf);

  ssize_t sent = send(to->sockfd, packet.buf, packet.size, 0);
  if (sent < 0) {
    return PS_ERROR_INTERNAL; 
  }

  return PS_SUCCESS;
}

#endif // __linux__
