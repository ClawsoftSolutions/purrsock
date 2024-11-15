#ifndef _WIN32
#  error "Windows is only supported for now"
#endif // _WIN32

#include "common.h"

#include <string.h>

#include <purrr/purrr.h>

#include <cglm/cglm.h>

#include <Windows.h>

static uint32_t g_indices[] = {
  0, 1, 2, 2, 3, 0
};

typedef struct {
  float pos[3];
  float uv[2];
} vertex_t;

static vertex_t g_vertices[] = {
  { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, },
  { {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, },
  { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }, },
  { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }, },
};

static HANDLE s_game_info_mutex = NULL;
static game_info_t s_game_info = {0};
static size_t s_id;

static struct {
  purrr_window_t *window;
  purrr_renderer_info_t renderer_info;
  purrr_renderer_t *renderer;

  purrr_pipeline_t *pipeline;
  purrr_mesh_info_t rect_mesh_info;
  purrr_mesh_t *rect_mesh;

  struct {
    mat4 projection;
    mat4 view;
  } ubo;
  purrr_buffer_t *ubo_buffer;
  void *ubo_data;

  struct {
    mat4 *models;
    size_t capacity;
    size_t count;
  } ssbo;
  purrr_buffer_info_t ssbo_info;
  purrr_buffer_t *ssbo_buffer;
  void *ssbo_data;
} s_renderer = {0};

typedef struct {
  ps_socket_t socket;
  ps_socket_t server_socket;
} thread_data_t;

void renderer_init();
bool renderer_update();
void renderer_resize(void *data);
void renderer_destroy();

DWORD WINAPI thread_func(LPVOID);

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
    return 1;
  }

  ps_port_t port = 0;
  {
    char *end = NULL;
    port = (ps_port_t)strtoul(argv[2], &end, 0);
    if (end == argv[2]) {
      fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
      fprintf(stderr, "%s is not a valid port\n", argv[2]);
      return 1;
    }
  }

  if (!ps_init()) return 1;

  ps_result_t result = PS_SUCCESS;
  ps_socket_t client = {0};
  if (result = ps_create_socket(&client, PS_PROTOCOL_UDP)) return 1;

  ps_socket_t server = {0};
  if (result = ps_create_socket_from_addr(&server, PS_PROTOCOL_UDP, argv[1], port)) return 1;

  if (result = send_packet(client, server, (packet_t){ .kind = PACKET_HELLO })) return 1;

  packet_t packet = {0};
  if (result = recv_packet(client, &server, &packet)) return 1;
  assert(packet.kind == PACKET_HELLO);
  s_id = packet.id;
  printf("Id: %zu\n", s_id);

  s_game_info_mutex = CreateMutex(NULL, FALSE, NULL);
  if (!s_game_info_mutex) return 1;

  DWORD thread_id = 0;
  HANDLE thread_handle = NULL;

  {
    thread_data_t thread_data = {
      .socket = client,
      .server_socket = server,
    };
    thread_handle = CreateThread(NULL, 0, thread_func, &thread_data, 0, &thread_id);
    if (!thread_handle) return 1;
  }

  renderer_init();
  renderer_resize(s_renderer.window);

  while (!purrr_window_should_close(s_renderer.window)) {
    {
      int8_t x = purrr_window_is_key_down(s_renderer.window, PURRR_KEY_D)-purrr_window_is_key_down(s_renderer.window, PURRR_KEY_A);
      int8_t y = purrr_window_is_key_down(s_renderer.window, PURRR_KEY_S)-purrr_window_is_key_down(s_renderer.window, PURRR_KEY_W);
      if (x != 0 || y != 0) {
        if (send_packet(client, server, (packet_t){
          .id = s_id,
          .kind = PACKET_MOVE,
          .size = sizeof(int8_t)*2,
          .buf = (char*)((int8_t[]){ x, y })
        })) break;
      }
    }

    purrr_renderer_begin_frame(s_renderer.renderer);

    purrr_renderer_begin_render_target(s_renderer.renderer, s_renderer.renderer_info.swapchain_render_target);
    purrr_renderer_bind_pipeline(s_renderer.renderer, s_renderer.pipeline);

    assert(WaitForSingleObject(s_game_info_mutex, INFINITE) != WAIT_ABANDONED);
    if (!renderer_update()) break;
    purrr_renderer_bind_buffer(s_renderer.renderer, s_renderer.ubo_buffer, 0);
    purrr_renderer_bind_buffer(s_renderer.renderer, s_renderer.ssbo_buffer, 1);
    for (size_t i = 0; i < s_game_info.players.count; ++i) {
      purrr_renderer_push_constant(s_renderer.renderer, 0, sizeof(size_t), &i);
      purrr_renderer_draw_mesh(s_renderer.renderer, s_renderer.rect_mesh);
    }
    assert(ReleaseMutex(s_game_info_mutex));

    purrr_renderer_end_render_target(s_renderer.renderer);

    purrr_renderer_end_frame(s_renderer.renderer);
    purrr_poll_events();
  }

  send_packet(client, server, (packet_t){ .id = s_id, .kind = PACKET_DISCONNECT });

  renderer_destroy();

  WaitForSingleObject(thread_handle, INFINITE);

  ps_cleanup();

  return 0;
}

void renderer_init() {
  purrr_window_info_t window_info = {
    .api = PURRR_API_VULKAN,
    .title = "purrsock example",
    .width = 800,
    .height = 600,
    .x = PURRR_WINDOW_POS_CENTER,
    .y = PURRR_WINDOW_POS_CENTER,
  };

  s_renderer.window = purrr_window_create(&window_info);
  assert(s_renderer.window);

  s_renderer.renderer_info = (purrr_renderer_info_t){
    .window = s_renderer.window,
    .vsync = true,
  };

  s_renderer.renderer = purrr_renderer_create(&s_renderer.renderer_info);
  assert(s_renderer.renderer);

  purrr_pipeline_shader_info_t shaders[] = {
    (purrr_pipeline_shader_info_t){
      .buffer = "../examples/purrr/vertex.spv",
      .type = PURRR_SHADER_TYPE_VERTEX
    },
    (purrr_pipeline_shader_info_t){
      .buffer = "../examples/purrr/fragment.spv",
      .type = PURRR_SHADER_TYPE_FRAGMENT
    },
  };

  purrr_vertex_info_t vertex_infos[] = {
    (purrr_vertex_info_t){
      .format = PURRR_FORMAT_RGB32F,
      .size = 12,
      .offset = 0,
    },
    (purrr_vertex_info_t){
      .format = PURRR_FORMAT_RG32F,
      .size = 8,
      .offset = 12,
    },
  };

  purrr_pipeline_info_t pipeline_info = {
    .shader_infos = shaders,
    .shader_info_count = 2,
    .pipeline_descriptor = s_renderer.renderer_info.swapchain_pipeline_descriptor,
    .mesh_info = (purrr_mesh_binding_info_t){
      .vertex_infos = vertex_infos,
      .vertex_info_count = 2,
    },
    .descriptor_slots = (purrr_descriptor_type_t[]){ PURRR_DESCRIPTOR_TYPE_UNIFORM_BUFFER, PURRR_DESCRIPTOR_TYPE_STORAGE_BUFFER },
    .descriptor_slot_count = 2,
    .push_constants = (purrr_pipeline_push_constant_t[]){ (purrr_pipeline_push_constant_t){ .offset = 0, .size = sizeof(size_t) } },
    .push_constant_count = 1,
  };

  s_renderer.pipeline = purrr_pipeline_create(&pipeline_info, s_renderer.renderer);
  assert(s_renderer.pipeline);

  s_renderer.rect_mesh_info = (purrr_mesh_info_t){
    .index_count = 6,
    .indices_size = sizeof(g_indices),
    .indices = g_indices,
    .vertices_size = sizeof(g_vertices),
    .vertices = g_vertices,
  };

  s_renderer.rect_mesh = purrr_mesh_create(&s_renderer.rect_mesh_info, s_renderer.renderer);
  assert(s_renderer.rect_mesh);

  purrr_renderer_set_resize_callback(s_renderer.renderer, renderer_resize);

  purrr_buffer_info_t ubo_info = {
    .type = PURRR_BUFFER_TYPE_UNIFORM,
    .size = sizeof(s_renderer.ubo)
  };

  s_renderer.ubo_buffer = purrr_buffer_create(&ubo_info, s_renderer.renderer);
  assert(s_renderer.ubo_buffer);
  assert(purrr_buffer_map(s_renderer.ubo_buffer, &s_renderer.ubo_data));

  s_renderer.ssbo_info = (purrr_buffer_info_t){
    .type = PURRR_BUFFER_TYPE_STORAGE,
    .size = 0
  };

  purrr_renderer_set_user_data(s_renderer.renderer, s_renderer.window);
}

bool renderer_update() {
  glm_lookat_rh((vec3){ 0.0f, 0.0f, 1.0f }, (vec3){0}, (vec3){ 0.0f, 1.0f, 0.0f }, s_renderer.ubo.view);

  if (s_game_info.players.count > s_renderer.ssbo.count)
    s_renderer.ssbo.count = s_game_info.players.count;

  if (s_renderer.ssbo.count >= s_renderer.ssbo.capacity || !s_renderer.ssbo_buffer) {
    if (s_renderer.ssbo.capacity) s_renderer.ssbo.capacity *= 2;
    else s_renderer.ssbo.capacity = 4;
    s_renderer.ssbo.models = (mat4*)realloc(s_renderer.ssbo.models, sizeof(mat4)*s_renderer.ssbo.capacity);
    if (!s_renderer.ssbo.models) return false;

    s_renderer.ssbo_info.size = sizeof(*s_renderer.ssbo.models)*s_renderer.ssbo.capacity;

    if (s_renderer.ssbo_buffer) purrr_buffer_unmap(s_renderer.ssbo_buffer);
    s_renderer.ssbo_buffer = purrr_buffer_create(&s_renderer.ssbo_info, s_renderer.renderer);
    assert(s_renderer.ssbo_buffer);
    purrr_buffer_map(s_renderer.ssbo_buffer, &s_renderer.ssbo_data);
  }

  for (size_t i = 0; i < s_game_info.players.count; ++i) {
    mat4 model_mat = {0};
    glm_mat4_identity(model_mat);
    glm_translate(model_mat, (vec3){ s_game_info.players.items[i].x, s_game_info.players.items[i].y, 0.0f });
    glm_scale(model_mat, (vec3){0.1f, 0.1f, 0.1f});
    glm_mat4_ucopy(model_mat, s_renderer.ssbo.models[i]);
  }

  memcpy(s_renderer.ubo_data, &s_renderer.ubo, sizeof(s_renderer.ubo));
  memcpy(s_renderer.ssbo_data, s_renderer.ssbo.models, sizeof(*s_renderer.ssbo.models)*s_renderer.ssbo.capacity);

  return true;
}

void renderer_resize(void *data) {
  purrr_window_t *window = (purrr_window_t*)data;
  assert(window);

  uint32_t width, height;
  purrr_window_get_size(window, &width, &height);

  float aspect = width/(float)height;
  glm_ortho_rh_no(-aspect, aspect, -1, 1, -1, 1, s_renderer.ubo.projection);
}

void renderer_destroy() {
  purrr_renderer_wait(s_renderer.renderer);

  purrr_buffer_destroy(s_renderer.ubo_buffer);
  purrr_buffer_destroy(s_renderer.ssbo_buffer);
  purrr_mesh_destroy(s_renderer.rect_mesh);
  purrr_pipeline_destroy(s_renderer.pipeline);
  purrr_renderer_destroy(s_renderer.renderer);
  purrr_window_destroy(s_renderer.window);
}

DWORD WINAPI thread_func(LPVOID lpData) {
  if (!lpData) return 1;
  thread_data_t data = *(thread_data_t*)lpData;

  packet_t packet = {0};

  ps_result_t result = PS_SUCCESS;
  bool playing = true;
  while (playing) {
    result = recv_packet(data.socket, &data.server_socket, &packet);
    if (result) break;

    if (packet.kind != PACKET_JOIN && packet.kind != PACKET_LEAVE && packet.kind != PACKET_MOVE) continue;
    assert(WaitForSingleObject(s_game_info_mutex, INFINITE) != WAIT_ABANDONED);

    if (packet.id >= s_game_info.players.count) s_game_info.players.count = packet.id+1;
    while (s_game_info.players.count >= s_game_info.players.capacity) {
      if (s_game_info.players.capacity) s_game_info.players.capacity *= 2;
      else s_game_info.players.capacity = 4;
      s_game_info.players.items = (pos_t*)realloc(s_game_info.players.items, s_game_info.players.capacity);
      assert(s_game_info.players.items);
    }

    switch (packet.kind) {
    case PACKET_JOIN:
    case PACKET_MOVE: {
      assert(packet.size == sizeof(pos_t));
      pos_t pos = *(pos_t*)packet.buf;
      s_game_info.players.items[packet.id] = pos;
    } break;
    case PACKET_LEAVE: {
      if (packet.id == s_id) playing = false; // It's time for us
      else printf("Player %zu left!\n", packet.id);
    } break;
    }

    assert(ReleaseMutex(s_game_info_mutex));
  }

  return 0;
}