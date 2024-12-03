#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <windows.h>
#include "purrsock/purrsock.h"


static void test_initialization(void** state) {
    (void)state;
    assert_true(ps_init());
}

static void test_create_socket_tcp(void** state) {
    (void)state;
    ps_init();

    ps_socket_t socket;
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);

    if (result != PS_SUCCESS) {
        fail_msg("Socket creation failed!");
    }

    assert_int_equal(result, PS_SUCCESS);
    ps_destroy_socket(socket);
}

static void test_create_socket_udp(void** state) {
    (void)state;
    ps_init();

    ps_socket_t socket;
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_UDP, PS_ADDRESS_IPV4);

    assert_int_equal(result, PS_SUCCESS);
    ps_destroy_socket(socket);
}

static void test_bind_socket(void** state) {
    (void)state;
    ps_init();
    ps_socket_t socket;
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(socket);
}

static void test_listen_socket(void** state) {
    (void)state;
    ps_init();

    ps_socket_t socket;
    ps_result_t result = ps_create_socket(&socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV6);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(socket, "::1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(socket);
}


static void test_connect_socket(void** state) {
    (void)state;

    ps_socket_t client_socket, server_socket;
    ps_result_t result = ps_create_socket(&server_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV6);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(server_socket, "::1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(server_socket);
    assert_int_equal(result, PS_SUCCESS);
    
    result = ps_create_socket(&client_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV6);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_connect_socket(client_socket, "::1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(client_socket);
    ps_destroy_socket(server_socket);
}

typedef struct {
    ps_socket_t* server_socket;
    ps_socket_t* client_socket;
} thread_params_t;

static DWORD WINAPI send_server(LPVOID param) {
    thread_params_t* params = (thread_params_t*)param;
    ps_socket_t* server_socket = params->server_socket;
    ps_socket_t* client_socket = params->client_socket;

    ps_packet_t packet;
    ps_result_t result = ps_read_socket_packet(*server_socket, &packet, NULL);
    if (result == PS_SUCCESS) {
        printf("Received packet: %s\n", (char*)packet.buf);
    }

    return 0;
}

static void test_send_socket(void** state) {
    (void)state;
    ps_init();

    ps_socket_t server_socket;
    ps_result_t result = ps_create_socket(&server_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(server_socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(server_socket, 1);  // TCP listen
    assert_int_equal(result, PS_SUCCESS);

    ps_socket_t client_socket;
    result = ps_create_socket(&client_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_connect_socket(client_socket, "127.0.0.1", 8080);  // TCP connect
    assert_int_equal(result, PS_SUCCESS);
     
    result = ps_accept_socket(server_socket, &client_socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_packet_t packet = {
        .buf = "Test data",
        .size = strlen("Test data") + 1,
        .capacity = 256
    };

    thread_params_t params;
    params.server_socket = &server_socket;
    params.client_socket = &client_socket;

    HANDLE server_thread_handle = CreateThread(
        NULL,
        0,
        send_server,
        &params,
        0,
        NULL
    );

    Sleep(100);

    result = ps_send_socket_packet(client_socket, packet, NULL);
    if (result != PS_SUCCESS) {
        printf("Failed to send packet, error code: %d\n", result);
    }
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(client_socket);

    WaitForSingleObject(server_thread_handle, INFINITE);

    ps_destroy_socket(server_socket);

    CloseHandle(server_thread_handle);
}



static DWORD WINAPI read_server(LPVOID param) {
    thread_params_t* params = (thread_params_t*)param;
    ps_socket_t* server_socket = params->server_socket;
    ps_socket_t* client_socket = params->client_socket;

    ps_result_t result = ps_accept_socket(*server_socket, client_socket);
    if (result != PS_SUCCESS) {
        printf("Failed to accept client connection, error code: %d\n", result);
        return 1;
    }

    ps_packet_t packet = { 0 };
    result = ps_read_socket_packet(*server_socket, &packet, NULL);
    if (result == PS_SUCCESS) {
        printf("Received packet: %s\n", (char*)packet.buf);
    }
    else {
        printf("Failed to receive packet, error code: %d\n", result);
    }

    return 0;
}

static void test_read_socket(void** state) {
    (void)state;
    ps_init();

    ps_socket_t server_socket;
    ps_result_t result = ps_create_socket(&server_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(server_socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(server_socket, 1);  // TCP listen
    assert_int_equal(result, PS_SUCCESS);

    ps_packet_t send_packet = {
        .buf = "Test data",
        .size = strlen("Test data") + 1,
        .capacity = 256
    };

    ps_socket_t client_socket;
    result = ps_create_socket(&client_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_connect_socket(client_socket, "127.0.0.1", 8080);  // TCP connect
    assert_int_equal(result, PS_SUCCESS);

    thread_params_t params;
    params.server_socket = &server_socket;
    params.client_socket = &client_socket;

    HANDLE server_thread_handle = CreateThread(
        NULL,
        0,
        read_server,
        &params,
        0,
        NULL
    );

    result = ps_send_socket_packet(client_socket, send_packet, NULL);
    if (result != PS_SUCCESS) {
        printf("Failed to send packet, error code: %d\n", result);
    }
    assert_int_equal(result, PS_SUCCESS);

    WaitForSingleObject(server_thread_handle, INFINITE);

    ps_destroy_socket(client_socket);
    ps_destroy_socket(server_socket);

    CloseHandle(server_thread_handle);
}




static void test_accept_socket(void** state) {
    (void)state;
    ps_init();

    ps_socket_t server_socket;
    ps_result_t result = ps_create_socket(&server_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_bind_socket(server_socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_listen_socket(server_socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_socket_t client_socket;
    result = ps_create_socket(&client_socket, PS_PROTOCOL_TCP, PS_ADDRESS_IPV4);
    assert_int_equal(result, PS_SUCCESS);

    result = ps_connect_socket(client_socket, "127.0.0.1", 8080);
    assert_int_equal(result, PS_SUCCESS);

    ps_socket_t accepted_socket;
    result = ps_accept_socket(server_socket, &accepted_socket);
    assert_int_equal(result, PS_SUCCESS);

    ps_destroy_socket(client_socket);
    ps_destroy_socket(accepted_socket);
    ps_destroy_socket(server_socket);
}

static void test_cleanup(void** state) {
    (void)state;
    ps_cleanup();
    assert_true(true);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_initialization),
        cmocka_unit_test(test_create_socket_tcp),
        cmocka_unit_test(test_create_socket_udp),
        cmocka_unit_test(test_bind_socket),
        cmocka_unit_test(test_listen_socket),
        cmocka_unit_test(test_connect_socket),
        cmocka_unit_test(test_send_socket),
        cmocka_unit_test(test_read_socket),
        cmocka_unit_test(test_accept_socket),
        cmocka_unit_test(test_cleanup),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
