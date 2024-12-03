// Copyright (c) 2024 ClawsoftSolutions. All rights reserved.
//
// This software is licensed under the MIT License.
// See LICENSE file for more information.
//

#ifdef _WIN32

#include "internal.h"

// https://learn.microsoft.com/en-us/windows/win32/api/_winsock/
#include <winsock2.h>
#include <Windows.h>
#include <af_irda.h>
#include <in6addr.h>
#include <ws2spi.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
  WSADATA wsadata;
} _purrsock_data_t;

static _purrsock_data_t *s_purrsock_data = NULL;

static CRITICAL_SECTION lock;

bool _purrsock_init() {
    static bool lock_initialized = false;

    if (!lock_initialized) {
        InitializeCriticalSection(&lock);
        lock_initialized = true;
    }
    EnterCriticalSection(&lock);
    if (s_purrsock_data) {
        LeaveCriticalSection(&lock);
        return true;
    }
    s_purrsock_data = malloc(sizeof(*s_purrsock_data));
    if (!s_purrsock_data) {
        printf("Memory allocation failed.\n");
        LeaveCriticalSection(&lock);
        return false;
    }
    memset(s_purrsock_data, 0, sizeof(*s_purrsock_data));
    int wsa_result = WSAStartup(MAKEWORD(2, 2), &s_purrsock_data->wsadata);
    if (wsa_result != 0) {
        printf("WSAStartup failed.\n");
        logWSAError(wsa_result);
        free(s_purrsock_data);
        s_purrsock_data = NULL;
        LeaveCriticalSection(&lock);
        return false;
    }
    LeaveCriticalSection(&lock);
    return true;
}




void _purrsock_cleanup() {
    EnterCriticalSection(&lock);
    if (s_purrsock_data) {
        WSACleanup();
        free(s_purrsock_data);
        s_purrsock_data = NULL;
    }
    LeaveCriticalSection(&lock);
}

ps_result_t _last_ps_result(const char *func_name) {
    int error = WSAGetLastError();
    ps_result_t result = PS_ERROR_UNKNOWN;

    switch (error) {
        case 0: result = PS_SUCCESS; break;
        case WSANOTINITIALISED: result = PS_ERROR_NOTINIT; break;
        case WSAEMSGSIZE: result = PS_ERROR_MSGTOOLONG; break;
        case WSAEADDRINUSE: result = PS_ERROR_ADDRINUSE; break;
        case WSAEADDRNOTAVAIL: result = PS_ERROR_ADDRNOTAVAIL; break;
        case WSAENETDOWN: result = PS_ERROR_NETDOWN; break;
        case WSAENETRESET: result = PS_ERROR_NETRESET; break;
        case WSAECONNRESET: result = PS_ERROR_CONNRESET; break;
        case WSAECONNREFUSED: result = PS_ERROR_CONNREFUSED; break;
        case WSAEHOSTDOWN: result = PS_ERROR_HOSTDOWN; break;
        case WSAESHUTDOWN: result = PS_ERROR_SHUTDOWN; break;
        case WSAETIMEDOUT: result = PS_ERROR_TIMEOUT; break;

        case WSAEAFNOSUPPORT: result = PS_ERROR_IPV6_ADDR_INVALID; break;
        case WSAENOTSOCK: result = PS_ERROR_IPV6_SOCKET_CREATION; break;
        case WSAEOPNOTSUPP: result = PS_ERROR_IPV6_SOCKET_BINDING; break;

        default:
            result = PS_ERROR_UNKNOWN;
            logWSAError(error);
            return result;
    }
    assert(result != PS_ERROR_UNKNOWN && "Unexpected error result.");
    
    return result;
}

void logWSAError(int wsaError) {
    
    if (wsaError != 0) {
        char *errorMessage = NULL;
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, 
            wsaError, 
            0, 
            (LPSTR)&errorMessage, 
            0, 
            NULL
        );
        printf("WSA Error Code: %d - %s\n", wsaError, errorMessage);
        LocalFree(errorMessage);
    }
}


typedef struct {
    SOCKET socket;
    union {
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
    };
} _purrsock_socket_data_t;


ps_result_t _purrsock_create_socket(_purrsock_socket_t* in_socket) {
    assert(in_socket);

    _purrsock_socket_data_t* data = (_purrsock_socket_data_t*)malloc(sizeof(*data));
    if (!data) {
        perror("malloc failed");
        return PS_ERROR_INTERNAL;
    }
    memset(data, 0, sizeof(*data));

    ps_result_t result = PS_SUCCESS;

    int address_family;
    switch (in_socket->addr_storage.ss_family) {
    case PS_ADDRESS_IPV4:
        address_family = AF_INET;
        break;
    case PS_ADDRESS_IPV6:
        address_family = AF_INET6;
        break;
    default:
        printf("Invalid address family: %d\n", in_socket->addr_storage.ss_family);
        free(data);
        return PS_ERROR_INVALID_ARGUMENT;
    }

    switch (in_socket->protocol) {
    case PS_PROTOCOL_TCP:
        data->socket = socket(address_family, SOCK_STREAM, IPPROTO_TCP);
        break;
    case PS_PROTOCOL_UDP:
        data->socket = socket(address_family, SOCK_DGRAM, IPPROTO_UDP);
        break;
    default:
        printf("Unreachable protocol: %d\n", in_socket->protocol);
        free(data);
        return PS_ERROR_INTERNAL;
    }

    if (data->socket == INVALID_SOCKET) {
        perror("Socket creation failed");
        free(data);
        return _last_ps_result("purrsock_create_socket");
    }

    if (address_family == AF_INET6) {
        int option = 0;
        if (setsockopt(data->socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&option, sizeof(option)) == SOCKET_ERROR) {
            perror("Failed to set IPV6_V6ONLY option");
        }
    }

    in_socket->data = data;

    return result;
}



ps_result_t _purrsock_create_socket_from_addr(_purrsock_socket_t *in_socket, const char *ip, ps_port_t port) {
  assert(in_socket);

  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)malloc(sizeof(*data));
  assert(data);
  memset(data, 0, sizeof(*data));

  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));

  if (inet_pton(AF_INET6, ip, &addr.sin6_addr) <= 0) {
    struct sockaddr_in addr4;
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(port);
    addr4.sin_addr.s_addr = inet_addr(ip);
    in_socket->data = data;
    return PS_SUCCESS;
  } else {
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    in_socket->data = data;
  }

  return PS_SUCCESS;
}

void _purrsock_destroy_socket(_purrsock_socket_t *socket) {
    assert(socket);
    _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
    if (data) {
        if (data->socket != INVALID_SOCKET) closesocket(data->socket);
        free(data);
    }
    socket->data = NULL;
}


ps_result_t _purrsock_bind_socket(_purrsock_socket_t* socket, const char* ip, ps_port_t port) {
    assert(socket);

    _purrsock_socket_data_t* data = (_purrsock_socket_data_t*)socket->data;
    if (!data) return PS_ERROR_NOTINIT;

    struct sockaddr_in6 addr6;
    struct sockaddr_in addr4;

    memset(&addr4, 0, sizeof(addr4));
    memset(&addr6, 0, sizeof(addr6));

    if (ip) {
        if (inet_pton(AF_INET6, ip, &addr6.sin6_addr) == 1) {
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(port);
            return bind(data->socket, (struct sockaddr*)&addr6, sizeof(addr6)) == SOCKET_ERROR
                ? _last_ps_result("purrsock_bind_socket (IPv6)")
                : PS_SUCCESS;
        }
        else if (inet_pton(AF_INET, ip, &addr4.sin_addr) == 1) {
            addr4.sin_family = AF_INET;
            addr4.sin_port = htons(port);
            return bind(data->socket, (struct sockaddr*)&addr4, sizeof(addr4)) == SOCKET_ERROR
                ? _last_ps_result("purrsock_bind_socket (IPv4)")
                : PS_SUCCESS;
        }

    }
    else {
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(port);

        if (bind(data->socket, (struct sockaddr*)&addr6, sizeof(addr6)) == SOCKET_ERROR) {
            addr4.sin_family = AF_INET;
            addr4.sin_addr.s_addr = htonl(INADDR_ANY);
            addr4.sin_port = htons(port);
            if (bind(data->socket, (struct sockaddr*)&addr4, sizeof(addr4)) == SOCKET_ERROR) {
                return _last_ps_result("purrsock_bind_socket (default IPv4)");
            }
        }
    }

    return PS_SUCCESS;
}



ps_result_t _purrsock_listen_socket(_purrsock_socket_t* socket) {
    assert(socket && socket->protocol == PS_PROTOCOL_TCP);

    _purrsock_socket_data_t* data = (_purrsock_socket_data_t*)socket->data;
    if (!data) return PS_ERROR_NOTINIT;

    printf("Attempting to listen on socket %d...\n", data->socket);

    if (listen(data->socket, SOMAXCONN) == SOCKET_ERROR) {
        return _last_ps_result("purrsock_listen_socket");
    }

    printf("Socket %d is now listening.\n", data->socket);
    return PS_SUCCESS;
}


ps_result_t _purrsock_accept_socket(_purrsock_socket_t* socket, _purrsock_socket_t** client) {
    assert(socket && client);
    _purrsock_socket_data_t* data = (_purrsock_socket_data_t*)socket->data;
    if (!data) return PS_ERROR_NOTINIT;

    struct sockaddr_in6 addr6 = { 0 };
    struct sockaddr_in addr4 = { 0 };
    int addr_size = sizeof(addr6);

    SOCKET sock = accept(data->socket, (struct sockaddr*)&addr6, &addr_size);

    if (sock == INVALID_SOCKET) {
        addr_size = sizeof(addr4);
        sock = accept(data->socket, (struct sockaddr*)&addr4, &addr_size);
        if (sock == INVALID_SOCKET) return _last_ps_result("purrsock_accept_socket");
    }

    *client = (_purrsock_socket_t*)malloc(sizeof(**client));
    assert(*client);
    (*client)->protocol = socket->protocol;

    _purrsock_socket_data_t* client_data = (_purrsock_socket_data_t*)malloc(sizeof(*client_data));
    assert(client_data);
    client_data->socket = sock;

    if (addr6.sin6_family == AF_INET6) {
        client_data->addr6 = addr6;
    }
    else if (addr4.sin_family == AF_INET) {
        client_data->addr4 = addr4;
    }

    (*client)->data = client_data;

    return PS_SUCCESS;
}


ps_result_t _purrsock_connect_socket(_purrsock_socket_t *socket, const char *ip, ps_port_t port) {
  assert(socket);
  _purrsock_socket_data_t *data = (_purrsock_socket_data_t*)socket->data;
  if (!data) return PS_ERROR_NOTINIT;

  struct sockaddr_in6 addr6 = {0};
  struct sockaddr_in addr4 = {0};

  if (inet_pton(AF_INET6, ip, &addr6.sin6_addr) > 0) {
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(port);
    if (connect(data->socket, (struct sockaddr*)&addr6, sizeof(addr6)) == SOCKET_ERROR) {
      return _last_ps_result("purrsock_connect_socket IPv6");
    }
  } else {
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(port);
    addr4.sin_addr.s_addr = inet_addr(ip);
    if (connect(data->socket, (struct sockaddr*)&addr4, sizeof(addr4)) == SOCKET_ERROR) {
      return _last_ps_result("purrsock_connect_socket IPv4");
    }
  }

  return PS_SUCCESS;
}

ps_result_t _purrsock_read_socket_packet(_purrsock_socket_t* socket, ps_packet_t* packet, _purrsock_socket_t** from) {
    assert(socket && packet);
    _purrsock_socket_data_t* data = (_purrsock_socket_data_t*)socket->data;
    if (!data) return PS_ERROR_NOTINIT;

    int res = 0;
    switch (socket->protocol) {
    case PS_PROTOCOL_TCP: {
        res = recv(data->socket, packet->buf, packet->capacity, 0);
    } break;
    case PS_PROTOCOL_UDP: {
        assert(from);
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
        int fromlen = sizeof(addr4);
        res = recvfrom(data->socket, packet->buf, packet->capacity, 0, (struct sockaddr*)&addr4, &fromlen);
        if (res <= 0) {
            fromlen = sizeof(addr6);
            res = recvfrom(data->socket, packet->buf, packet->capacity, 0, (struct sockaddr*)&addr6, &fromlen);
            if (res > 0) {
                *from = (_purrsock_socket_t*)malloc(sizeof(**from));
                assert(*from);

                _purrsock_socket_data_t* from_data = (_purrsock_socket_data_t*)malloc(sizeof(*from_data));
                assert(from_data);

                from_data->addr6 = addr6;
                (*from)->protocol = socket->protocol;
                (*from)->data = from_data;
            }
        }
        else {
            *from = (_purrsock_socket_t*)malloc(sizeof(**from));
            assert(*from);

            _purrsock_socket_data_t* from_data = (_purrsock_socket_data_t*)malloc(sizeof(*from_data));
            assert(from_data);

            from_data->addr4 = addr4;
            (*from)->protocol = socket->protocol;
            (*from)->data = from_data;
        }
    } break;
    default: return PS_ERROR_INTERNAL;
    }

    if (res == SOCKET_ERROR) return _last_ps_result("purrsock_read_socket_packet");

    packet->size = res;
    return PS_SUCCESS;
}

ps_result_t _purrsock_send_socket_packet(_purrsock_socket_t* socket, ps_packet_t* packet, _purrsock_socket_t* to) {
    assert(socket && packet);

    if (to) {
        _purrsock_socket_data_t* to_data = (_purrsock_socket_data_t*)(to)->data;
        if (!to_data) return PS_ERROR_NOTINIT;
    }

    _purrsock_socket_data_t* data = (_purrsock_socket_data_t*)socket->data;
    if (!data) return PS_ERROR_NOTINIT;

    int res = 0;
    switch (socket->protocol) {
    case PS_PROTOCOL_TCP: {
        res = send(data->socket, packet->buf, packet->size, 0);
    } break;
    case PS_PROTOCOL_UDP: {
        if (!to) return PS_ERROR_INVALID_ARGUMENT;

        _purrsock_socket_data_t* to_data = (_purrsock_socket_data_t*)(to)->data;
        if (!to_data) return PS_ERROR_NOTINIT;

        if (to_data->addr4.sin_family == AF_INET) {
            struct sockaddr_in addr4 = { 0 };
            addr4.sin_family = AF_INET;
            addr4 = to_data->addr4;
            res = sendto(data->socket, packet->buf, packet->size, 0, (struct sockaddr*)&addr4, sizeof(addr4));
        }
        else if (to_data->addr6.sin6_family == AF_INET6) {
            struct sockaddr_in6 addr6 = { 0 };
            addr6.sin6_family = AF_INET6;
            addr6 = to_data->addr6;
            res = sendto(data->socket, packet->buf, packet->size, 0, (struct sockaddr*)&addr6, sizeof(addr6));
        }
    } break;
    default:
        return PS_ERROR_INTERNAL;
    }

    if (res == SOCKET_ERROR) return _last_ps_result("purrsock_send_socket_packet");
    return PS_SUCCESS;
}



#endif
