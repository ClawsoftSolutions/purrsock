#ifndef   PURRR_COMMON_H_
#define   PURRR_COMMON_H_

#include "purrsock/purrsock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef enum {
  PACKET_HELLO = 0,
  PACKET_DISCONNECT,
  PACKET_JOIN,
  PACKET_LEAVE,
  PACKET_MOVE,
} packet_kind_t;

typedef struct {
  size_t id;
  packet_kind_t kind;
  size_t size;
  char *buf;
} packet_t;

typedef struct {
  float x, y;
} pos_t;

typedef struct {
  pos_t *items;
  size_t capacity;
  size_t count;
} players_t;

typedef struct {
  players_t players;
} game_info_t;

ps_result_t send_packet(ps_socket_t from, ps_socket_t to, packet_t packet) {
  ps_packet_t ps_packet = {0};
  ps_packet.buf = malloc(ps_packet.size = sizeof(packet_t)-sizeof(char*)+packet.size);
  memcpy(&ps_packet.buf[ 0], &packet.id, sizeof(packet.id));
  memcpy(&ps_packet.buf[ 8], &packet.size, sizeof(packet.size));
  memcpy(&ps_packet.buf[16], &packet.kind, sizeof(packet.kind));
  memcpy(&ps_packet.buf[20], packet.buf, packet.size);
  ps_result_t result = ps_send_socket_packet(from, ps_packet, to);
  free(ps_packet.buf);
  return result;
}

ps_result_t recv_packet(ps_socket_t socket, ps_socket_t *from, packet_t *packet) {
  ps_packet_t ps_packet = {0};
  ps_packet.buf = malloc(ps_packet.capacity = 20+1024); // 1024 = max buf size
  ps_result_t result = ps_read_socket_packet(socket, &ps_packet, from);
  if (result) return result;
  packet->id = ((size_t*)ps_packet.buf)[0];
  packet->size = ((size_t*)ps_packet.buf)[1];
  packet->kind = *(packet_kind_t*)&ps_packet.buf[16];
  packet->buf = malloc(packet->size);
  memcpy(packet->buf, &ps_packet.buf[20], packet->size);
  free(ps_packet.buf);
  return PS_SUCCESS;
}

#endif // PURRR_COMMON_H_