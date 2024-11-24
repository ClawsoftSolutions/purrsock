// Copyright (c) 2024 ClawsoftSolutions. All rights reserved.
//
// This software is licensed under the MIT License.
// See LICENSE file for more information.
//



#ifndef   PURRSOCK_H_
#define   PURRSOCK_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enum representing possible result codes for socket operations.
 */
typedef enum {
  PS_CONNCLOSED = -2,          /**< Connection closed. */
  PS_ERROR_INTERNAL = -1,      /**< Internal error. */
  PS_SUCCESS = 0,              /**< Operation was successful. */
  PS_ERROR_NOTINIT,            /**< Socket library not initialized. */
  PS_ERROR_MSGTOOLONG,         /**< Message size is too long. */
  PS_ERROR_ADDRINUSE,          /**< Address is already in use. */
  PS_ERROR_ADDRNOTAVAIL,       /**< Address is not available. */
  PS_ERROR_NETDOWN,            /**< Network is down. */
  PS_ERROR_NETRESET,           /**< Network was reset. */
  PS_ERROR_CONNRESET,          /**< Connection was reset. */
  PS_ERROR_CONNREFUSED,        /**< Connection was refused. */
  PS_ERROR_HOSTDOWN,           /**< Host is down. */
  PS_ERROR_SHUTDOWN,           /**< Socket was shut down. */
  PS_ERROR_TIMEOUT,            /**< Timeout occurred. */

  /* IPv6 Specific Errors */
  PS_ERROR_IPV6_ADDR_PARSE,    /**< Error parsing IPv6 address. */
  PS_ERROR_IPV6_ADDR_INVALID,  /**< Invalid IPv6 address. */
  PS_ERROR_IPV6_CONNECT_FAILED,/**< Failed to connect via IPv6. */
  PS_ERROR_IPV6_SOCKET_CREATION, /**< Error creating IPv6 socket. */
  PS_ERROR_IPV6_SOCKET_BINDING,  /**< Error binding IPv6 socket. */
  PS_ERROR_IPV6_SOCKET_CLOSED,   /**< IPv6 socket was closed unexpectedly. */
  
  PS_ERROR_UNKNOWN             /**< Unknown error code. */
} ps_result_t;

/**
 * @brief Converts a `ps_result_t` result code to a human-readable string.
 *
 * @param result The result code to convert.
 * @return A string representation of the result code.
 */
const char *ps_result_to_cstr(ps_result_t result);

/**
 * @brief Enum representing supported protocols for socket communication.
 */
typedef enum {
  PS_PROTOCOL_TCP = 0,         /**< Transmission Control Protocol. */
  PS_PROTOCOL_UDP,             /**< User Datagram Protocol. */

  COUNT_PS_PROTOCOLS          /**< Count of protocols. */
} ps_protocol_t;

/**
 * @brief Type definition for a port number.
 */
typedef uint16_t ps_port_t;

/**
 * @brief Initializes the socket library.
 * 
 * This function must be called before using any socket operations.
 *
 * @return `true` if initialization is successful, `false` otherwise.
 */
bool ps_init();

/**
 * @brief Cleans up the socket library.
 * 
 * This function must be called to clean up resources after using socket operations.
 */
void ps_cleanup();

/**
 * @brief Opaque structure representing a socket.
 * 
 * The actual details of this structure are hidden from the user.
 */
typedef struct ps_socket_s *ps_socket_t;

/**
 * @brief Creates a socket of the specified protocol.
 * 
 * @param socket Pointer to a variable that will hold the created socket.
 * @param protocol The protocol to use for the socket (TCP or UDP).
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_create_socket(ps_socket_t *socket, ps_protocol_t protocol);

/**
 * @brief Creates a socket from an address and port.
 * 
 * @param socket Pointer to a variable that will hold the created socket.
 * @param protocol The protocol to use for the socket (TCP or UDP).
 * @param ip The IP address to bind the socket to.
 * @param port The port to bind the socket to.
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_create_socket_from_addr(ps_socket_t *socket, ps_protocol_t protocol, const char *ip, ps_port_t port);

/**
 * @brief Destroys a socket, releasing its resources.
 * 
 * @param socket The socket to destroy.
 */
void ps_destroy_socket(ps_socket_t socket);

/**
 * @brief Binds a socket to a specific IP address and port.
 * 
 * @param socket The socket to bind.
 * @param ip The IP address to bind to.
 * @param port The port to bind to.
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_bind_socket(ps_socket_t socket, const char *ip, ps_port_t port);

/**
 * @brief Puts the socket into listening mode, waiting for incoming connections.
 * 
 * @param socket The socket to listen on.
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_listen_socket(ps_socket_t socket);

/**
 * @brief Accepts an incoming connection on a listening socket.
 * 
 * @param socket The listening socket.
 * @param client Pointer to a variable that will hold the accepted client socket.
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_accept_socket(ps_socket_t socket, ps_socket_t *client);

/**
 * @brief Connects a socket to a remote server.
 * 
 * @param socket The socket to connect.
 * @param ip The IP address of the remote server.
 * @param port The port of the remote server.
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_connect_socket(ps_socket_t socket, const char *ip, ps_port_t port);

/**
 * @brief Structure representing a socket packet, used for sending/receiving data.
 */
typedef struct {
  size_t size;    /**< The size of the data in the packet. */
  char *buf;      /**< The buffer containing the data. */
  size_t capacity; /**< The capacity of the buffer, needed for packet management. */
} ps_packet_t;

/**
 * @brief Reads a packet of data from a socket.
 * 
 * @param socket The socket to read data from.
 * @param packet Pointer to a `ps_packet_t` structure to hold the read data.
 * @param from Pointer to a `ps_socket_t` variable to hold the sender's socket (optional).
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_read_socket_packet(ps_socket_t socket, ps_packet_t *packet, ps_socket_t *from);

/**
 * @brief Sends a packet of data through a socket.
 * 
 * @param socket The socket to send data through.
 * @param packet The packet of data to send.
 * @param to The socket to send the data to.
 * @return A `ps_result_t` result code indicating the success or failure of the operation.
 */
ps_result_t ps_send_socket_packet(ps_socket_t socket, ps_packet_t packet, ps_socket_t to);

#endif // PURRSOCK_H_
