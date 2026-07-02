#include "MiniFB.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

extern "C" {
#include "microui.h"
}
#include "ui_bridge.h"
#include "ui_renderer.h"

#define WIDTH 1600
#define HEIGHT 1200

static uint32_t g_buffer[WIDTH * HEIGHT];
static uint32_t g_line_buffer[WIDTH * HEIGHT] = {0};
static float circle_density = 1.0f; 
static int show_normals = 0; 
static int use_perspective = 0;
static float cam_x = 0, cam_y = 0, cam_z = 5.0f;
static float cam_rx = 0, cam_ry = 0; 
static int show_axes = 0;
static int show_bbox = 0;
static int show_bbox_raster = 0;
static float world_tx = 0, world_ty = 0;
static float world_ry = 0;
static float local_sx = 1, local_sy = 1;
static bool drawing_line = false;
static int line_start_x = 0, line_start_y = 0;
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {
      g_line_buffer[y0 * WIDTH + x0] = color;
    }
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
void draw_line_bg(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy;
    while(true) {
        if(x0>=0 && x0<WIDTH && y0>=0 && y0<HEIGHT)
            g_buffer[y0*WIDTH+x0] = color;
        if(x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if(e2>=dy){err+=dy; x0+=sx;}
        if(e2<=dx){err+=dx; y0+=sy;}
    }
}
glm::vec2 to_screen(glm::vec4 clip) {
    float x = (clip.x / clip.w + 1.0f) * WIDTH / 2.0f;
    float y = (1.0f - clip.y / clip.w) * HEIGHT / 2.0f;
    return glm::vec2(x, y);
}
// 3D data structures
struct Vec3 { float x, y, z; };
struct Face { int a, b, c; };

std::vector<Vec3> g_vertices;
std::vector<Face> g_faces;

void load_obj(const char* filename) {
    g_vertices.clear();
    g_faces.clear();
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string type;
        ss >> type;
        if (type == "v") {
            Vec3 v;
            ss >> v.x >> v.y >> v.z;
            g_vertices.push_back(v);
        } else if (type == "f") {
            Face f;
            ss >> f.a >> f.b >> f.c;
            f.a--; f.b--; f.c--;
            g_faces.push_back(f);
            
        }
    }
    printf("Loaded: %zu vertices, %zu faces\n", g_vertices.size(), g_faces.size());
}
void normalize_mesh(float screen_width, float screen_height) {
    if (g_vertices.empty()) return;

    float min_x = g_vertices[0].x, max_x = g_vertices[0].x;
    float min_y = g_vertices[0].y, max_y = g_vertices[0].y;

    for (auto& v : g_vertices) {
        min_x = std::min(min_x, v.x); max_x = std::max(max_x, v.x);
        min_y = std::min(min_y, v.y); max_y = std::max(max_y, v.y);
    }

    float range_x = max_x - min_x;
    float range_y = max_y - min_y;
    float scale = 0.7f * std::min(screen_width / range_x, screen_height / range_y);

    float cx = (min_x + max_x) / 2.0f;
    float cy = (min_y + max_y) / 2.0f;

    for (auto& v : g_vertices) {
        v.x = (v.x - cx) * scale + screen_width / 2.0f;
        v.y = (v.y - cy) * scale + screen_height / 2.0f;
        v.z = v.z * scale;
    }
    printf("Normalized mesh to fit screen!\n");
}
struct Normal { float x, y, z; };
std::vector<Normal> g_face_normals;
std::vector<glm::vec3> g_face_centers;

void compute_normals() {
    g_face_normals.clear();
    g_face_centers.clear();
    for (auto& face : g_faces) {
        Vec3& v0 = g_vertices[face.a];
        Vec3& v1 = g_vertices[face.b];
        Vec3& v2 = g_vertices[face.c];

        glm::vec3 edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
        glm::vec3 edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        g_face_normals.push_back({normal.x, normal.y, normal.z});
        g_face_centers.push_back(glm::vec3(
            (v0.x + v1.x + v2.x) / 3.0f,
            (v0.y + v1.y + v2.y) / 3.0f,
            (v0.z + v1.z + v2.z) / 3.0f
        ));
    }
}

int main() {
  // GLM demo - Part 0
  glm::vec3 position(1.0f, 2.0f, 3.0f);
  glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
  printf("GLM works! Position: (%.1f, %.1f, %.1f)\n", position.x, position.y, position.z);
  load_obj("cube.obj");
  // Scale and center the mesh
  normalize_mesh(400.0f, 400.0f);
  compute_normals();
  struct mfb_window *window =
      mfb_open_ex("MiniGUI Platform", WIDTH, HEIGHT, MFB_WF_RESIZABLE);
  if (!window)
    return 1;

  mu_Context *ctx = (mu_Context *)malloc(sizeof(mu_Context));
  mu_init(ctx);

  // Set font callbacks for microui
  ctx->text_width = [](mu_Font font, const char *str, int len) {
    return (len < 0 ? (int)strlen(str) : len) * 8;
  };
  ctx->text_height = [](mu_Font font) { return 8; };

  UIRenderer renderer(WIDTH, HEIGHT);

  // Set up char input callback for textbox input
static int g_color_shift = 0;

  // Set up char input callback for textbox input
  mfb_set_char_input_callback(
      [](struct mfb_window *w, unsigned int c) {
        if (c == 'r') {
          g_color_shift = rand() % 255;
        } else {
          extern void ui_bridge_char_input(struct mfb_window *, unsigned int);
          ui_bridge_char_input(w, c);
        }
      },
      window);
  mfb_set_keyboard_callback(
      [](struct mfb_window *w, mfb_key key, mfb_key_mod mod, bool isPressed) {
       if (key == KB_KEY_LEFT)  world_tx -= 0.1f;
       if (key == KB_KEY_RIGHT) world_tx += 0.1f;
       if (key == KB_KEY_UP)    world_ty -= 0.1f;
       if (key == KB_KEY_DOWN)  world_ty += 0.1f;
      },
      window);    


  while (mfb_update_events(window) != MFB_STATE_EXIT) {
    // 1. Input
    ui_bridge_input(ctx, window);

    // 2. Scene Rendering (Background)
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
      // Simple gradient background
      int x = i % WIDTH;
      int y = i / WIDTH;
      int cx = WIDTH / 2;
      int cy = HEIGHT / 2;
      float dist = sqrt((float)((x - cx) * (x - cx) + (y - cy) * (y - cy))) * circle_density;
      uint8_t r = (uint8_t)(dist * 0.5f + g_color_shift) % 255;
      uint8_t g = (uint8_t)(dist * 0.3f + 80) % 255;
      uint8_t b = (uint8_t)(255 - (dist * 0.4f)) % 255;     
      g_buffer[i] = MFB_RGB(r, g, b);
    }
    // Merge line buffer
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
      if (g_line_buffer[i] != 0) {
        g_buffer[i] = g_line_buffer[i];
      }
    }

    // Part 5: Apply transformations and draw wireframe
    glm::mat4 local = glm::mat4(1.0f);
    local = glm::scale(local, glm::vec3(local_sx, local_sy, 1.0f));

    glm::mat4 world = glm::mat4(1.0f);
    world = glm::rotate(world, world_ry, glm::vec3(0, 1, 0));
    world = glm::translate(world, glm::vec3(world_tx, world_ty, 0));

    // Part 2: View matrix (camera)
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::rotate(view, -cam_rx, glm::vec3(1, 0, 0));
    view = glm::rotate(view, -cam_ry, glm::vec3(0, 1, 0));
    view = glm::translate(view, glm::vec3(-cam_x, -cam_y, -cam_z));

   // Part 3: Projection matrix
    glm::mat4 proj;

    if (use_perspective)
   {
    proj = glm::perspective(
        glm::radians(60.0f),
        (float)WIDTH / HEIGHT,
        0.1f,
        10000.0f);
}
else
{
    proj = glm::ortho(
        -800.0f, 800.0f,
        -600.0f, 600.0f,
        0.1f, 10000.0f);
}

glm::mat4 final_transform = proj * view * world * local;
    for (auto& face : g_faces) {
        Vec3& v0 = g_vertices[face.a];
        Vec3& v1 = g_vertices[face.b];
        Vec3& v2 = g_vertices[face.c];

        glm::vec4 c0 = final_transform * glm::vec4(v0.x, v0.y, v0.z, 1.0f);
        glm::vec4 c1 = final_transform * glm::vec4(v1.x, v1.y, v1.z, 1.0f);
        glm::vec4 c2 = final_transform * glm::vec4(v2.x, v2.y, v2.z, 1.0f);

        glm::vec2 s0 = to_screen(c0);
        glm::vec2 s1 = to_screen(c1);
        glm::vec2 s2 = to_screen(c2);

        draw_line_bg((int)s0.x, (int)s0.y, (int)s1.x, (int)s1.y, MFB_RGB(255, 255, 255));
        draw_line_bg((int)s1.x, (int)s1.y, (int)s2.x, (int)s2.y, MFB_RGB(255, 255, 255));
        draw_line_bg((int)s2.x, (int)s2.y, (int)s0.x, (int)s0.y, MFB_RGB(255, 255, 255));
    }
    // hw4 Part 1: Bounding box rasterization
    if (show_bbox_raster) {
        srand(42);
        for (auto& face : g_faces) {
            Vec3& v0 = g_vertices[face.a];
            Vec3& v1 = g_vertices[face.b];
            Vec3& v2 = g_vertices[face.c];

            glm::vec4 c0 = final_transform * glm::vec4(v0.x, v0.y, v0.z, 1.0f);
            glm::vec4 c1 = final_transform * glm::vec4(v1.x, v1.y, v1.z, 1.0f);
            glm::vec4 c2 = final_transform * glm::vec4(v2.x, v2.y, v2.z, 1.0f);
            if (c0.w <= 0 || c1.w <= 0 || c2.w <= 0) continue;
            glm::vec2 s0 = to_screen(c0);
            glm::vec2 s1 = to_screen(c1);
            glm::vec2 s2 = to_screen(c2);

            int minX = std::max(0, (int)std::min({s0.x, s1.x, s2.x}));
            int maxX = std::min(WIDTH-1, (int)std::max({s0.x, s1.x, s2.x}));
            int minY = std::max(0, (int)std::min({s0.y, s1.y, s2.y}));
            int maxY = std::min(HEIGHT-1, (int)std::max({s0.y, s1.y, s2.y}));

            uint32_t color = MFB_RGB(rand()%255, rand()%255, rand()%255);
            for (int y = minY; y <= maxY; y++)
                for (int x = minX; x <= maxX; x++)
                    g_buffer[y * WIDTH + x] = color;
        }
    }
    // Part 1: Draw coordinate axes
    if (show_axes)
{
    glm::vec4 origin = final_transform * glm::vec4(0, 0, 0, 1);
    glm::vec4 x_axis = final_transform * glm::vec4(100, 0, 0, 1);
    glm::vec4 y_axis = final_transform * glm::vec4(0, 100, 0, 1);
    glm::vec4 z_axis = final_transform * glm::vec4(0, 0, 100, 1);

    if (origin.w > 0 && x_axis.w > 0 && y_axis.w > 0 && z_axis.w > 0)
    {
        glm::vec2 so = to_screen(origin);
        glm::vec2 sx = to_screen(x_axis);
        glm::vec2 sy = to_screen(y_axis);
        glm::vec2 sz = to_screen(z_axis);

        draw_line_bg((int)so.x, (int)so.y,
                     (int)sx.x, (int)sx.y,
                     MFB_RGB(255, 0, 0));

        draw_line_bg((int)so.x, (int)so.y,
                     (int)sy.x, (int)sy.y,
                     MFB_RGB(0, 255, 0));

        draw_line_bg((int)so.x, (int)so.y,
                     (int)sz.x, (int)sz.y,
                     MFB_RGB(0, 0, 255));
    }
}
      // Part 4: Draw face normals
    // Part 4: Draw face normals
    if (show_normals) {
      for (int i = 0; i < (int)g_faces.size(); i++) {
         

        glm::vec3 center = g_face_centers[i];

glm::vec4 c = final_transform * glm::vec4(center, 1.0f);
glm::vec4 tip = final_transform * glm::vec4(
    center.x + g_face_normals[i].x * 200,
    center.y + g_face_normals[i].y * 200,
    center.z + g_face_normals[i].z * 200,
    1.0f);

if (c.w > 0 && tip.w > 0)
{
    glm::vec2 sc = to_screen(c);
    glm::vec2 st = to_screen(tip);

    draw_line_bg(
        (int)sc.x,
        (int)sc.y,
        (int)st.x,
        (int)st.y,
        MFB_RGB(255,255,0));
}
    }
}

    // 3. UI Logic
  

    static float slider_val = 50.0f;
    static float number_val = 3.14f;
    static int checkbox_a = 0;
    static int checkbox_b = 1;
    static char textbox_buf[128] = "edit me";
    static bool quit_requested = false;
    mu_begin(ctx);

    // --- Widgets window ---
    if (mu_begin_window(ctx, "Widgets", mu_rect(20, 20, 360, 540))) {
      int w1[] = {-1};

      // label / text
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_label: plain static text");
      mu_text(ctx, "mu_text: word-wrapped longer text that will reflow inside "
                   "the window width automatically.");

      // button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "mu_button: click me")) {
        quit_requested = false; // just a reaction
      }
      // my new button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "Say Hello!")) {
        printf("Hello from my custom button!\n");
    }
      // Part 1: Show mesh info
      mu_layout_row(ctx, 1, w1, 0);
      char mesh_info[64];
      snprintf(mesh_info, sizeof(mesh_info), "Vertices: %zu  Faces: %zu", g_vertices.size(), g_faces.size());
      mu_label(ctx, mesh_info);
      // checkbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "mu_checkbox A (off)", &checkbox_a);
      mu_checkbox(ctx, "mu_checkbox B (on)", &checkbox_b);
      
          // Part 2: Camera controls
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "-- Camera Position --");
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &cam_x, -1000, 1000);
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &cam_y, -1000, 1000);
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &cam_z, -1000, 1000);
      mu_label(ctx, "-- Camera Rotation --");
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &cam_rx, -3.14f, 3.14f);
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &cam_ry, -3.14f, 3.14f);
      // Part 3: Projection toggle
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Perspective Projection", &use_perspective);
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Show Normals", &show_normals);

      // textbox
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_textbox:");
      mu_textbox(ctx, textbox_buf, sizeof(textbox_buf));

      // slider
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_slider (0-100):");
      mu_slider(ctx, &slider_val, 0, 100);
      // my circle density slider
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "Circle density:");
      mu_slider(ctx, &circle_density, 0.1f, 5.0f);
      // Part 4: Transformation sliders
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "-- World Translation --");
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &world_tx, -5, 5);
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &world_ty, -5, 5);
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "-- World Rotation Y --");
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &world_ry, -3.14f, 3.14f);
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "-- Local Scale --");
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &local_sx, 0.1f, 3.0f);
      mu_layout_row(ctx, 1, w1, 0);
      mu_slider(ctx, &local_sy, 0.1f, 3.0f);

      // Part 1: Debug toggles
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "-- Debug --");
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Show Axes", &show_axes);
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Show Bounding Box", &show_bbox);
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "-- hw4 Rasterization --");
      mu_layout_row(ctx, 1, w1, 0);
      mu_checkbox(ctx, "Show BBox Raster", &show_bbox_raster);

      // number
      mu_layout_row(ctx, 1, w1, 0);
      mu_label(ctx, "mu_number (step 0.1):");
      mu_number(ctx, &number_val, 0.1f);

      // header (collapsible section)
      if (mu_header(ctx, "mu_header: collapsible section")) {
        mu_layout_row(ctx, 1, w1, 0);
        mu_label(ctx, "Content inside the header.");
      }

      // treenode
      if (mu_begin_treenode(ctx, "mu_treenode: root")) {
        mu_layout_row(ctx, 1, w1, 0);
        mu_label(ctx, "child item A");
        if (mu_begin_treenode(ctx, "nested node")) {
          mu_layout_row(ctx, 1, w1, 0);
          mu_label(ctx, "deeply nested item");
          mu_end_treenode(ctx);
        }
        mu_end_treenode(ctx);
      }

      // quit button
      mu_layout_row(ctx, 1, w1, 0);
      if (mu_button(ctx, "Quit")) {
        quit_requested = true;
      }

      mu_end_window(ctx);
    }

    // --- Panel window ---
    if (mu_begin_window(ctx, "Panel Demo", mu_rect(395, 20, 380, 200))) {
      int w2[] = {-1};
      mu_layout_row(ctx, 1, w2, 120);
      mu_begin_panel(ctx, "scrollable panel");
      int wp[] = {-1};
      for (int i = 1; i <= 12; i++) {
        mu_layout_row(ctx, 1, wp, 0);
        char line[32];
        snprintf(line, sizeof(line), "Panel row %d", i);
        mu_label(ctx, line);
      }
      mu_end_panel(ctx);
      mu_end_window(ctx);
    }

    // --- Popup demo window ---
    if (mu_begin_window(ctx, "Popup Demo", mu_rect(395, 235, 380, 80))) {
      int w3[] = {-1};
      mu_layout_row(ctx, 1, w3, 0);
      if (mu_button(ctx, "Open popup")) {
        mu_Container *popup = mu_get_container(ctx, "my popup");
        popup->rect = mu_rect(ctx->mouse_pos.x, ctx->mouse_pos.y, 260, 84);
        popup->open = 1;
        ctx->hover_root = ctx->next_hover_root = popup;
        mu_bring_to_front(ctx, popup);
      }
      int popup_opt = MU_OPT_POPUP | MU_OPT_NORESIZE | MU_OPT_NOSCROLL |
                      MU_OPT_NOTITLE | MU_OPT_CLOSED;
      if (mu_begin_window_ex(ctx, "my popup", mu_rect(0, 0, 260, 84),
                             popup_opt)) {
        int wp[] = {-1};
        mu_layout_row(ctx, 1, wp, 0);
        mu_label(ctx, "mu_popup: click outside to close");
        if (mu_button(ctx, "Close")) {
          mu_get_current_container(ctx)->open = 0;
        }
        mu_end_window(ctx);
      }
      mu_end_window(ctx);
    }

    mu_end(ctx);
    // Part 6: Interactive line drawing
    if (ctx->mouse_down == MU_MOUSE_LEFT && !drawing_line) {
  // mouse just pressed
      drawing_line = true;
      line_start_x = ctx->mouse_pos.x;
      line_start_y = ctx->mouse_pos.y;
    }
    if (drawing_line && ctx->mouse_down != MU_MOUSE_LEFT) {
  // mouse released - finalize the line
     draw_line(line_start_x, line_start_y, ctx->mouse_pos.x, ctx->mouse_pos.y, MFB_RGB(255, 255, 255));
     drawing_line = false;
  }

    if (quit_requested) {
      mfb_close(window);
      break;
    }

    // 4. UI Rendering
    renderer.render(ctx, g_buffer);

    // 5. Display
    mfb_update_state state = mfb_update_ex(window, g_buffer, WIDTH, HEIGHT);
    if (state < 0)
      break;

    // Cap FPS (optional, minifb has built-in sync)
    mfb_wait_sync(window);
  }

  mfb_close(window);
  free(ctx);
  return 0;
}
