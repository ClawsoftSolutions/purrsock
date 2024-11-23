#include <purrsock/purrsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Main server function for the purrsock example.
 * 
 * This program demonstrates a simple TCP server using the purrsock library. It binds a socket to a local IP and port,
 * listens for incoming connections, and handles client requests by sending HTTP responses. If the client sends a 
 * "GET" request, the server responds with a basic HTML page; otherwise, it responds with a 401 Unauthorized message.
 * 
 * @return int Returns 0 on success, 1 on failure.
 */
int main(void) {
    // Initialize purrsock library
    if (!ps_init()) return 1;  ///< Initialize the purrsock library (returns 0 if successful)

    ps_result_t result = PS_SUCCESS;
    ps_socket_t socket = {0};  ///< Socket for listening for incoming connections
    
    // Create a TCP socket
    if ((result = ps_create_socket(&socket, PS_PROTOCOL_TCP)) != PS_SUCCESS) goto cleanup;

    // Bind socket to localhost and port 6969
    if ((result = ps_bind_socket(socket, "127.0.0.1", 6969)) != PS_SUCCESS) goto cleanup;
    
    // Start listening for incoming connections
    if ((result = ps_listen_socket(socket)) != PS_SUCCESS) goto cleanup;

    ps_socket_t client_socket = {0};  ///< Client socket for handling communication
    while (1) {
        // Accept an incoming connection from a client
        if ((result = ps_accept_socket(socket, &client_socket)) != PS_SUCCESS) goto cleanup;

        ps_packet_t request = {0};  ///< Variable to store the incoming request
        request.buf = malloc(request.capacity = 1024);  ///< Allocate memory for the request buffer

        // Read the incoming packet from the client
        if ((result = ps_read_socket_packet(client_socket, &request, NULL)) != PS_SUCCESS) goto cleanup;

        // Check if the request is a "GET" request, otherwise return a 401 Unauthorized error
        if (memcmp(request.buf, "GET", 3) != 0) {
            ps_packet_t response = {0};  ///< Variable to store the response packet
            response.buf = "HTTP/1.1 401 Unauthorized\r\nContent-Type: text/plain; charset=ascii\r\nContent-Length: 13\r\n\r\nFuck off mate";
            response.size = 103;  ///< Set the size of the response buffer

            // Send the response to the client
            if ((result = ps_send_socket_packet(client_socket, response, NULL)) != PS_SUCCESS) goto cleanup;
        } else {
            const char *body = "<body><h1>purrsock example</h1></body>";  ///< HTML body content for a valid response

            ps_packet_t response = {0};  ///< Variable to store the response packet for a valid request
            response.buf = malloc(2048);  ///< Allocate memory for the response buffer
            response.size = sprintf(response.buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=ascii\r\nContent-Length: %zu\r\n\r\n%s", strlen(body)-1, body);

            // Send the response (HTML page) to the client
            if ((result = ps_send_socket_packet(client_socket, response, NULL)) != PS_SUCCESS) goto cleanup;
        }
    }

cleanup:
    // Cleanup: close the socket and free resources
    if (socket) ps_destroy_socket(socket);

    // Final cleanup for purrsock library
    ps_cleanup();

    return 0;  ///< Return 0 to indicate successful execution
}
