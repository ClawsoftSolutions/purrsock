#include "common.h"

typedef struct {
  size_t *items;
  size_t capacity;
  size_t count;
} free_ids_t;

static free_ids_t s_free_id_pool = {0};
static game_info_t s_game_info = {0};

static struct {
  ps_socket_t *items;
  size_t capacity;
  size_t count;
} s_clients = {0};

bool handle_packet(packet_t packet, ps_socket_t socket, ps_socket_t client);

int main(int argc, char **argv) {
  ps_port_t port = 6969;
  if (argc == 2) {
    char *end = NULL;
    port = (ps_port_t)strtoul(argv[1], &end, 0);
    if (end == argv[1]) {
      fprintf(stderr, "Usage: %s <port>\n", argv[0]);
      fprintf(stderr, "%s is not a valid port\n", argv[1]);
      return 1;
    }
  }

  if (!ps_init()) return 1;

  ps_result_t result = PS_SUCCESS;
  ps_socket_t socket = {0};
  if (result = ps_create_socket(&socket, PS_PROTOCOL_UDP)) goto defer;

  if (result = ps_bind_socket(socket, "127.0.0.1", port)) return 1;

  packet_t packet = {0};
  ps_socket_t client = {0};
  while (!result) {
    result = recv_packet(socket, &client, &packet);
    if (result) break;
    if (!handle_packet(packet, socket, client)) {
      result = PS_ERROR_INTERNAL;
      break;
    }
    free(packet.buf);
  }
defer:
  int ret_val = result != PS_SUCCESS;
  if (ret_val) fprintf(stderr, "%s\n", ps_result_to_cstr(result));

  if (socket) ps_destroy_socket(socket);

  ps_cleanup();

  return ret_val;
}

bool handle_packet(packet_t packet, ps_socket_t socket, ps_socket_t client) {
  switch (packet.kind) {
  case PACKET_HELLO: {
    size_t id = 0;
    if (s_free_id_pool.count > 0) id = s_free_id_pool.items[--s_free_id_pool.count];
    else {
      if (s_game_info.players.count >= s_game_info.players.capacity) {
        if (s_game_info.players.capacity) s_game_info.players.capacity *= 2;
        else s_game_info.players.capacity = 4;
        s_game_info.players.items = (pos_t*)realloc(s_game_info.players.items, s_game_info.players.capacity);
        assert(s_game_info.players.items);
      }

      if (s_clients.count >= s_clients.capacity) {
        if (s_clients.capacity) s_clients.capacity *= 2;
        else s_clients.capacity = 4;
        s_clients.items = (ps_socket_t*)realloc(s_clients.items, s_clients.capacity);
        assert(s_clients.items);
      }

      id = s_game_info.players.count++;
      ++s_clients.count;
    }

    assert(id < s_game_info.players.count);
    assert(id < s_clients.count);
    s_game_info.players.items[id] = (pos_t){ 0.0f, 0.0f };
    s_clients.items[id] = client;

    if (send_packet(socket, client, (packet_t){
      .id = id,
      .kind = PACKET_HELLO
    })) return false;

    { // Send information about new player to every player
      packet_t join_packet = {
        .id = id,
        .kind = PACKET_JOIN,
        .size = sizeof(pos_t),
        .buf = (char*)&s_game_info.players.items[id]
      };

      for (size_t i = 0; i < s_clients.count; ++i) {
        ps_socket_t s = s_clients.items[i];
        if (!s) continue;
        if (send_packet(socket, s, join_packet)) return false;
      }
    }

    { // Send information about every player (except the new one) to new player
      for (size_t i = 0; i < s_game_info.players.count; ++i) {
        if (i == id) continue;
        packet_t join_packet = {
          .id = i,
          .kind = PACKET_JOIN,
          .size = sizeof(pos_t),
          .buf = (char*)&s_game_info.players.items[i]
        };
        
        if (send_packet(socket, client, join_packet)) return false;
      }
    }
  } break;
  case PACKET_DISCONNECT: {
    if (s_free_id_pool.count >= s_free_id_pool.capacity) {
      if (s_free_id_pool.capacity) s_free_id_pool.capacity *= 2;
      else s_free_id_pool.capacity = 2;
      s_free_id_pool.items = (size_t*)realloc(s_free_id_pool.items, s_free_id_pool.capacity);
      assert(s_free_id_pool.items);
    }
    s_free_id_pool.items[s_free_id_pool.count++] = packet.id;

    packet_t leave_packet = { .id = packet.id, .kind = PACKET_LEAVE };

    for (size_t i = 0; i < s_clients.count; ++i) {
      ps_socket_t s = s_clients.items[i];
      if (!s) continue;
      if (send_packet(socket, s, leave_packet)) return false;
    }

    s_clients.items[packet.id] = NULL;
  } break;
  // I could send snapshots instead of sending every change but I don't really feel like doing it.
  // If you have too much time and you know C then you can submit a pull request or something.
  case PACKET_MOVE: {
    assert(packet.id < s_game_info.players.count);
    assert(packet.size == sizeof(int8_t)*2);
    int8_t x = ((int8_t*)packet.buf)[0];
    int8_t y = ((int8_t*)packet.buf)[1];
    s_game_info.players.items[packet.id].x += x*0.0001;
    s_game_info.players.items[packet.id].y += y*0.0001;

    packet_t move_packet = packet;
    move_packet.size = sizeof(pos_t);
    move_packet.buf = (char*)&s_game_info.players.items[packet.id];
    for (size_t i = 0; i < s_clients.count; ++i) {
      ps_socket_t s = s_clients.items[i];
      if (!s) continue;
      if (send_packet(socket, s, move_packet)) return false;
    }   
  } break;
  default:
    printf("Packet %zu not handled!\n", packet.kind);
  }

  return true;
}