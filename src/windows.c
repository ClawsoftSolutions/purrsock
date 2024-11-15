#ifdef   _WIN32

#include "internal.h"

// https://learn.microsoft.com/en-us/windows/win32/api/_winsock/
#include <winsock2.h>
#include <Windows.h>
#include <af_irda.h>
#include <in6addr.h>
#include <ws2spi.h>
#include <ws2tcpip.h>

#include <assert.h>

typedef struct {
  WSADATA wsadata;
} _purrsock_data_t;

static _purrsock_data_t *s_purrsock_data = NULL;

bool _purrsock_init() {
  if (s_purrsock_data) return true;
  s_purrsock_data = (_purrsock_data_t*)malloc(sizeof(*s_purrsock_data));
  memset(s_purrsock_data, 0, sizeof(*s_purrsock_data));
  return WSAStartup(MAKEWORD(2, 2), &s_purrsock_data->wsadata) == 0;
}

void _purrsock_cleanup() {
  assert(s_purrsock_data);
  WSACleanup();
}

ps_result_t _last_ps_result() {
  int error = WSAGetLastError();
  switch (error) {
  case 0: return PS_SUCCESS;

  case WSANOTINITIALISED: return PS_ERROR_NOTINIT;
  case WSAEMSGSIZE:       return PS_ERROR_MSGTOOLONG;
  case WSAEADDRINUSE:     return PS_ERROR_ADDRINUSE;
  case WSAEADDRNOTAVAIL:  return PS_ERROR_ADDRNOTAVAIL;
  case WSAENETDOWN:       return PS_ERROR_NETDOWN;
  case WSAENETRESET:      return PS_ERROR_NETRESET;
  case WSAECONNRESET:     return PS_ERROR_CONNRESET;
  case WSAECONNREFUSED:   return PS_ERROR_CONNREFUSED;
  case WSAEHOSTDOWN:      return PS_ERROR_HOSTDOWN;
  case WSAESHUTDOWN:      return PS_ERROR_SHUTDOWN;
  }
  return PS_ERROR_UNKNOWN;
}

typedef struct {
  SOCKET socket;
  struct sockaddr_in addr;
} _purrsock_socket_data_t;

ps_result_t _purrsock_create_socket(_purrsock_socket_t *in_socket) {
  assert(in_socket);

  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)malloc(sizeof(*data));
  assert(data);
  memset(data, 0, sizeof(*data));

  ps_result_t result = PS_SUCCESS;
  switch (in_socket->protocol) {
  case PS_PROTOCOL_TCP: {
    data->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  } break;
  case PS_PROTOCOL_UDP: {
    data->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  } break;
  default: {
    assert(0 && "Unreachable");
    result = PS_ERROR_INTERNAL;
  }
  }

  if (data->socket == INVALID_SOCKET)
    result = _last_ps_result();

  in_socket->data = data;

  if (result) { // Error
    _purrsock_destroy_socket(in_socket);
    free(data);
  }

  return result;
}

ps_result_t _purrsock_create_socket_from_addr(_purrsock_socket_t *in_socket, const char *ip, ps_port_t port) {
  assert(in_socket);

  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)malloc(sizeof(*data));
  assert(data);
  memset(data, 0, sizeof(*data));

  data->addr.sin_family = AF_INET;
  data->addr.sin_addr.s_addr = ip?inet_addr(ip):INADDR_ANY;
  data->addr.sin_port = htons(port);

  in_socket->data = data;

  return PS_SUCCESS;
}

void _purrsock_destroy_socket(_purrsock_socket_t *socket) {
  assert(socket);
  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (data->socket) closesocket(data->socket);
}

ps_result_t _purrsock_bind_socket(_purrsock_socket_t *socket, const char *ip, ps_port_t port) {
  assert(socket);

  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (!data) return PS_ERROR_NOTINIT;

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip?inet_addr(ip):INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(data->socket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return _last_ps_result();
  return PS_SUCCESS;
}

ps_result_t _purrsock_listen_socket(_purrsock_socket_t *socket) {
  assert(socket && socket->protocol == PS_PROTOCOL_TCP);

  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (!data) return PS_ERROR_NOTINIT;

  if (listen(data->socket, SOMAXCONN) == SOCKET_ERROR) return _last_ps_result();

  return PS_SUCCESS;
}

ps_result_t _purrsock_accept_socket(_purrsock_socket_t *socket, _purrsock_socket_t **client) {
  assert(socket && client);
  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (!data) return PS_ERROR_NOTINIT;

  struct sockaddr_in addr = {0};
  int addr_size = sizeof(addr);
  SOCKET sock = accept(data->socket, (struct sockaddr*)&addr, &addr_size);
  if (sock == INVALID_SOCKET) return _last_ps_result();

  *client = (_purrsock_socket_t*)malloc(sizeof(**client));
  assert(*client);
  (*client)->protocol = socket->protocol;

  _purrsock_socket_data_t *client_data = (_purrsock_socket_data_t*)malloc(sizeof(*client_data));
  assert(client_data);
  client_data->socket = sock;
  (*client)->data = client_data;

  return PS_SUCCESS;
}

ps_result_t _purrsock_connect_socket(_purrsock_socket_t *socket, const char *ip, ps_port_t port) {
  assert(socket && socket->protocol == PS_PROTOCOL_TCP);

  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (!data) return PS_ERROR_NOTINIT;

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip?inet_addr(ip):INADDR_ANY;
  addr.sin_port = htons(port);

  if (connect(data->socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) return _last_ps_result();

  return PS_SUCCESS;
}

ps_result_t _purrsock_read_socket_packet(_purrsock_socket_t *socket, ps_packet_t *packet, _purrsock_socket_t **from) {
  assert(socket && packet);
  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (!data) return PS_ERROR_NOTINIT;

  int res = 0;
  switch (socket->protocol) {
  case PS_PROTOCOL_TCP: {
    res = recv(data->socket, packet->buf, packet->capacity, 0);
  } break;
  case PS_PROTOCOL_UDP: {
    assert(from);
    _purrsock_socket_data_t *from_data = (_purrsock_socket_data_t*)malloc(sizeof(*from_data));
    int fromlen = sizeof(struct sockaddr_in);
    res = recvfrom(data->socket, packet->buf, packet->capacity, 0, (struct sockaddr*)&from_data->addr, &fromlen);
    if (res > 0) {
      *from = (_purrsock_socket_t*)malloc(sizeof(**from));
      assert(*from);
      (*from)->protocol = socket->protocol;
      (*from)->data = from_data;
    }
  } break;
  default: {
    assert(0 && "Unreachable");
    return PS_ERROR_INTERNAL;
  }
  }

  if (res < 0) return _last_ps_result();
  else if (res == 0) return PS_CONNCLOSED;
  packet->size = (size_t)res;
  return PS_SUCCESS;
}

ps_result_t _purrsock_send_socket_packet(_purrsock_socket_t *socket, ps_packet_t packet, _purrsock_socket_t *to) {
  assert(socket && packet.buf);
  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (!data) return PS_ERROR_NOTINIT;

  int res = 0;
  char *ptr = packet.buf;
  while (packet.size > 0 && res >= 0) {
    switch (socket->protocol) {
    case PS_PROTOCOL_TCP: {
      res = send(data->socket, ptr, packet.size, 0);
    } break;
    case PS_PROTOCOL_UDP: {
      assert(to);
      _purrsock_socket_data_t *to_data = (_purrsock_socket_data_t*)to->data;
      int tolen = sizeof(to_data->addr);
      res = sendto(data->socket, ptr, packet.size, 0, (struct sockaddr*)&to_data->addr, tolen);
    } break;
    default: {
      assert(0 && "Unreachable");
      return PS_ERROR_INTERNAL;
    }
    }

    if (res < 0) break;
    packet.size -= res;
    ptr += res;
  }

  if (res == SOCKET_ERROR) return _last_ps_result();
  return PS_SUCCESS;
}

#endif // _WIN32