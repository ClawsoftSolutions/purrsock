#include <purrsock/purrsock.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  if (!ps_init()) return 1;

  ps_result_t result = PS_SUCCESS;
  ps_socket_t socket = {0};
  if ((result = ps_create_socket(&socket, PS_PROTOCOL_TCP)) != PS_SUCCESS) goto cleanup;

  if ((result = ps_bind_socket(socket, "127.0.0.1", 6969)) != PS_SUCCESS) goto cleanup;
  if ((result = ps_listen_socket(socket)) != PS_SUCCESS) goto cleanup;

  ps_socket_t client_socket = {0};
  while (1) {
    if ((result = ps_accept_socket(socket, &client_socket)) != PS_SUCCESS) goto cleanup;

    ps_packet_t request = {0};
    request.buf = malloc(request.capacity = 1024);
    if ((result = ps_read_socket_packet(client_socket, &request, NULL)) != PS_SUCCESS) goto cleanup;

    if (memcmp(request.buf, "GET", 3) != 0) {
      ps_packet_t response = {0};
      response.buf = "HTTP/1.1 401 Unauthorized\r\nContent-Type: text/plain; charset=ascii\r\nContent-Length: 13\r\n\r\nFuck off mate";
      response.size = 103;
      if ((result = ps_send_socket_packet(client_socket, response, NULL)) != PS_SUCCESS) goto cleanup;
    } else {
      const char *body = "<body><h1>purrsock example</h1></body>";

      ps_packet_t response = {0};
      response.buf = malloc(2048);
      response.size = sprintf(response.buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=ascii\r\nContent-Length: %zu\r\n\r\n%s", strlen(body)-1, body);
      if ((result = ps_send_socket_packet(client_socket, response, NULL)) != PS_SUCCESS) goto cleanup;
    }
  }

cleanup:
  if (socket) ps_destroy_socket(socket);

  ps_cleanup();

  return 0;
}