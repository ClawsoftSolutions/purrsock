#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <windows.h>
#include "purrsock/purrsock.h"

static void test_initialization(void **state) {
    (void)state;
    assert_true(ps_init());
}

static void test_create_socket_tcp(void **state) {
    (void)state;
    ps_socket_t socket = {0};
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);
    ps_destroy_socket(socket);
}

static void test_create_socket_udp(void **state) {
    (void)state;
    ps_socket_t socket = {0};
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_UDP);
    assert_int_equal(result, PS_SUCCESS);
    ps_destroy_socket(socket);
}

static void test_bind_socket(void **state) {
    (void)state;
    ps_init();
    ps_socket_t socket = {0};
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(socket);
}

static void test_listen_socket(void **state) {
    (void)state;
    ps_socket_t socket = {0};
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(socket);
}

static void test_connect_socket(void **state) {
    (void)state;
    ps_socket_t socket = {0};
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_connect_socket(socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(socket);
}

static void test_send_packet(void **state) {
    (void)state;
    ps_socket_t socket = {0};
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_socket_t client_socket = {0};
    result = ps_accept_socket(socket, &client_socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_packet_t packet = {5, "Hello", 10};
    result = ps_send_socket_packet(client_socket, packet, socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(client_socket);
    ps_destroy_socket(socket);
}

static void test_read_packet(void **state) {
    (void)state;
    ps_socket_t socket = {0};
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_socket_t client_socket = {0};
    result = ps_accept_socket(socket, &client_socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_packet_t packet = {5, "Hello", 10};
    result = ps_send_socket_packet(client_socket, packet, socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_packet_t received_packet;
    result = ps_read_socket_packet(socket, &received_packet, NULL);
    assert_int_equal(result, PS_SUCCESS);

    assert_int_equal(received_packet.size, 5);
    assert_string_equal(received_packet.buf, "Hello");

    ps_destroy_socket(client_socket);
    ps_destroy_socket(socket);
}

static void test_cleanup(void **state) {
    (void)state;
    ps_cleanup();
    assert_true(true);
}

static void test_bind_socket_address_in_use(void **state) {
    (void)state;
    ps_socket_t socket1, socket2;
    ps_result_t result;

    result = ps_create_socket(&socket1, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket1, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_create_socket(&socket2, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket2, "127.0.0.1", 8080);
    assert_int_equal(result, PS_ERROR_ADDRINUSE);

    ps_destroy_socket(socket1);
    ps_destroy_socket(socket2);
}

static DWORD WINAPI send_packet_thread(LPVOID arg) {
    ps_socket_t *client_socket = (ps_socket_t *)arg;
    ps_packet_t packet = {5, "Hello", 10};
    ps_result_t result = ps_send_socket_packet(*client_socket, packet, *client_socket);
    assert_int_equal(result, PS_SUCCESS);
    return 0;
}

static DWORD WINAPI read_packet_thread(LPVOID arg) {
    ps_socket_t *server_socket = (ps_socket_t *)arg;
    ps_packet_t received_packet;
    ps_result_t result = ps_read_socket_packet(*server_socket, &received_packet, NULL);
    assert_int_equal(result, PS_SUCCESS);
    assert_int_equal(received_packet.size, 5);
    assert_string_equal(received_packet.buf, "Hello");
    return 0;
}

static void test_send_receive_packet_multithreaded(void **state) {
    (void)state;
    ps_socket_t server_socket, client_socket;
    ps_result_t result = ps_create_socket(&server_socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(server_socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(server_socket);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_create_socket(&client_socket, PS_PROTOCOL_TCP);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_connect_socket(client_socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    ps_socket_t accepted_client_socket;
    result = ps_accept_socket(server_socket, &accepted_client_socket);
    assert_int_equal(result, PS_SUCCESS);

    HANDLE send_thread, recv_thread;

    send_thread = CreateThread(NULL, 0, send_packet_thread, (LPVOID)&client_socket, 0, NULL);
    recv_thread = CreateThread(NULL, 0, read_packet_thread, (LPVOID)&server_socket, 0, NULL);

    WaitForSingleObject(send_thread, INFINITE);
    WaitForSingleObject(recv_thread, INFINITE);

    CloseHandle(send_thread);
    CloseHandle(recv_thread);

    ps_destroy_socket(accepted_client_socket);
    ps_destroy_socket(client_socket);
    ps_destroy_socket(server_socket);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_initialization),
        cmocka_unit_test(test_create_socket_tcp),
        cmocka_unit_test(test_create_socket_udp),
        cmocka_unit_test(test_bind_socket),
        cmocka_unit_test(test_listen_socket),
        cmocka_unit_test(test_connect_socket),
        cmocka_unit_test(test_send_packet),
        cmocka_unit_test(test_read_packet),
        cmocka_unit_test(test_cleanup),
        cmocka_unit_test(test_bind_socket_address_in_use),
        cmocka_unit_test(test_send_receive_packet_multithreaded),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
