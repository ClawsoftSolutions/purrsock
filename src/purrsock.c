// Copyright (c) 2024 ClawsoftSolutions. All rights reserved.
//
// This software is licensed under the MIT License.
// See LICENSE file for more information.
//



#include "internal.h"

#include <stdlib.h>
#include <assert.h>

const char *ps_result_to_cstr(ps_result_t result) {
  switch (result) {
  case PS_CONNCLOSED:         return "Connection closed";
  case PS_ERROR_INTERNAL:     return "Internal error";
  case PS_SUCCESS:            return "Success";
  case PS_ERROR_NOTINIT:      return "Error not initialized";
  case PS_ERROR_MSGTOOLONG:   return "Error msg too long";
  case PS_ERROR_ADDRINUSE:    return "Error addr in use";
  case PS_ERROR_ADDRNOTAVAIL: return "Error addr not avail";
  case PS_ERROR_NETDOWN:      return "Error net down";
  case PS_ERROR_NETRESET:     return "Error net reset";
  case PS_ERROR_CONNRESET:    return "Error conn reset";
  case PS_ERROR_CONNREFUSED:  return "Error conn refused";
  case PS_ERROR_HOSTDOWN:     return "Error host down";
  case PS_ERROR_SHUTDOWN:     return "Error shut down";
  case PS_ERROR_UNKNOWN:      return "Unknown error";
  }
  assert(0 && "Unreachable");
  return NULL;
}

const char* get_platform() {
  #ifdef PLATFORM_WINDOWS
      return "Windows";
  #elif defined(PLATFORM_LINUX)
      return "Linux";
  #else
      return "Unknown";
  #endif
}

bool ps_init() {
  return _purrsock_init();
}

void ps_cleanup() {
  _purrsock_cleanup();
}

ps_result_t ps_create_socket(ps_socket_t *socket, ps_protocol_t protocol) {
  assert(socket);
  _purrsock_socket_t *internal_socket = (_purrsock_socket_t*)malloc(sizeof(*internal_socket));
  assert(internal_socket);
  internal_socket->protocol = protocol;
  *socket = (ps_socket_t)internal_socket;

  return _purrsock_create_socket(internal_socket);
}

ps_result_t ps_create_socket_from_addr(ps_socket_t *socket, ps_protocol_t protocol, const char *ip, ps_port_t port) {
  assert(socket);
  _purrsock_socket_t *internal_socket = (_purrsock_socket_t*)malloc(sizeof(*internal_socket));
  assert(internal_socket);
  internal_socket->protocol = protocol;
  *socket = (ps_socket_t)internal_socket;

  return _purrsock_create_socket_from_addr(internal_socket, ip, port);
}

void ps_destroy_socket(ps_socket_t socket) {
  assert(socket);
  _purrsock_destroy_socket((_purrsock_socket_t*)socket);
  free(socket);
}

ps_result_t ps_bind_socket(ps_socket_t socket, const char *ip, ps_port_t port) {
  return _purrsock_bind_socket((_purrsock_socket_t*)socket, ip, port);
}

ps_result_t ps_listen_socket(ps_socket_t socket) {
  return _purrsock_listen_socket((_purrsock_socket_t*)socket);
}

ps_result_t ps_accept_socket(ps_socket_t socket, ps_socket_t *client) {
  return _purrsock_accept_socket((_purrsock_socket_t*)socket, (_purrsock_socket_t**)client);
}

ps_result_t ps_connect_socket(ps_socket_t socket, const char *ip, ps_port_t port) {
  return _purrsock_connect_socket((_purrsock_socket_t*)socket, ip, port);
}

ps_result_t ps_read_socket_packet(ps_socket_t socket, ps_packet_t *packet, ps_socket_t *from) {
  return _purrsock_read_socket_packet((_purrsock_socket_t*)socket, packet, (_purrsock_socket_t**)from);
}

ps_result_t ps_send_socket_packet(ps_socket_t socket, ps_packet_t packet, ps_socket_t to) {
  return _purrsock_send_socket_packet((_purrsock_socket_t*)socket, packet, (_purrsock_socket_t*)to);
}