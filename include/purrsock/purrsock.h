#ifndef   PURRSOCK_H_
#define   PURRSOCK_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  PS_CONNCLOSED = -2,
  PS_ERROR_INTERNAL = -1,
  PS_SUCCESS = 0,
  PS_ERROR_NOTINIT,
  PS_ERROR_MSGTOOLONG,
  PS_ERROR_ADDRINUSE,
  PS_ERROR_ADDRNOTAVAIL,
  PS_ERROR_NETDOWN,
  PS_ERROR_NETRESET,
  PS_ERROR_CONNRESET,
  PS_ERROR_CONNREFUSED,
  PS_ERROR_HOSTDOWN,
  PS_ERROR_SHUTDOWN,

  PS_ERROR_UNKNOWN
} ps_result_t;

const char *ps_result_to_cstr(ps_result_t result);

typedef enum {
  PS_PROTOCOL_TCP = 0,
  PS_PROTOCOL_UDP,

  COUNT_PS_PROTOCOLS
} ps_protocol_t;

typedef uint16_t ps_port_t;

bool ps_init();
void ps_cleanup();

typedef struct ps_socket_s *ps_socket_t;

ps_result_t ps_create_socket(ps_socket_t *socket, ps_protocol_t protocol);
ps_result_t ps_create_socket_from_addr(ps_socket_t *socket, ps_protocol_t protocol, const char *ip, ps_port_t port);
void ps_destroy_socket(ps_socket_t socket);

ps_result_t ps_bind_socket(ps_socket_t socket, const char *ip, ps_port_t port);
ps_result_t ps_listen_socket(ps_socket_t socket);
ps_result_t ps_accept_socket(ps_socket_t socket, ps_socket_t *client);

ps_result_t ps_connect_socket(ps_socket_t socket, const char *ip, ps_port_t port);

typedef struct {
  size_t size;
  char *buf;
  size_t capacity; // Needed by ps_read_socket_packet
} ps_packet_t;

ps_result_t ps_read_socket_packet(ps_socket_t socket, ps_packet_t *packet, ps_socket_t *from);
ps_result_t ps_send_socket_packet(ps_socket_t socket, ps_packet_t packet, ps_socket_t to);

#ifdef __cplusplus
}
#endif

#endif // PURRSOCK_H_
