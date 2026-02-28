#include <math.h>
#include <stdarg.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION

#include "glad.h"
#include "GLFW/glfw3.h"
#include "khash.h"
#include "time.h"

#include "float.h"

#include "utils.h"
#include "mesh.h"
#include "log.h"
#include "thing.h"
#include "physics.h"

#include "ft2build.h"
#include "cglm/cglm.h"
#include "kvec.h"
#include FT_FREETYPE_H

#define TITLE "replacement"

/*
 * ============
 * @KEYBINDINGS
 * ============
 *
 * - We need to be able to fetch keybind by action, and action associated with
 * keybind.
 * - We also need to be able to support keys that just fire as long as they are
 * held, as well as those with specific callbacks.
 * - We need to iterate over both the above groups in one loop.
 */

typedef struct Mouse {
  double xpos, ypos;
} Mouse;

Mouse MOUSE;

void mousePosUpdate(void* window, double xpos, double ypos) {
  /* log_debug("%f %f", MOUSE.xpos, MOUSE.ypos); */
  MOUSE.xpos = xpos;
  MOUSE.ypos = ypos;
}

void mouseInit(void* window) {
  glfwSetCursorPosCallback(window, (GLFWcursorposfun)mousePosUpdate);
}

enum {
  K_MOVE_FORW,
  K_MOVE_LEFT,
  K_MOVE_RIGHT,
  K_MOVE_BACK,
  K_JUMP,

  K_EDIT,

  KPAUSE,

  K_SHOOT,
  K_AIM,
  K_RELOAD,

  K_BINDS,  // number of binds
};

enum {
  K_MOUSE_LEFT,
  K_MOUSE_MIDDLE,
  K_MOUSE_RIGHT,

  K_MOUSE_BINDS,
};

// default keybindings

typedef struct {
  int callback;
  int key;
  int pressed;
} Keybinding;

#define KEYBIND(_key, _callback) \
  {.key = _key, .callback = _callback, .pressed = 0}

enum {
  NO_CALLBACK,
  CALLBACK_TOGGLE,
};

Keybinding Mousebinds[K_MOUSE_BINDS] = {
    KEYBIND(GLFW_MOUSE_BUTTON_1, NO_CALLBACK),
    KEYBIND(GLFW_MOUSE_BUTTON_3, NO_CALLBACK),
    KEYBIND(GLFW_MOUSE_BUTTON_2, NO_CALLBACK),
};

Keybinding Keybinds[K_BINDS] = {

    KEYBIND(GLFW_KEY_W, NO_CALLBACK),      // forward
    KEYBIND(GLFW_KEY_A, NO_CALLBACK),      // left
    KEYBIND(GLFW_KEY_D, NO_CALLBACK),      // right
    KEYBIND(GLFW_KEY_S, NO_CALLBACK),      // back
    KEYBIND(GLFW_KEY_SPACE, NO_CALLBACK),  // jump

    KEYBIND(GLFW_KEY_E, CALLBACK_TOGGLE),  // editor toggle

    KEYBIND(GLFW_KEY_ESCAPE, CALLBACK_TOGGLE),  // pause

    KEYBIND(GLFW_MOUSE_BUTTON_LEFT, CALLBACK_TOGGLE),   // shoot
    KEYBIND(GLFW_MOUSE_BUTTON_RIGHT, CALLBACK_TOGGLE),  // aim
    KEYBIND(GLFW_KEY_R, CALLBACK_TOGGLE),               // reload
};

#define KPRESSED(int) Keybinds[int].pressed
#define KRELEASE(int) Keybinds[int].pressed = 0

#define MPRESSED(int) Mousebinds[int].pressed
#define MRELEASE(int) Mousebinds[int].pressed = 0

// Check for key presses and respect callbacks.
// TODO: we check every single entry on a keypress. Create a mapping between key
// and action to save checks.
void keybindingsPoll(void* window, int key, int scancode, int action,
                     int mods) {
  int k, c;
  for (int i = 0; i < K_BINDS; i++) {
    k = Keybinds[i].key;
    c = Keybinds[i].callback;

    if (key == k) {
      switch (c) {
        case NO_CALLBACK:
          Keybinds[i].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
          break;
        case CALLBACK_TOGGLE:
          Keybinds[i].pressed = (action == GLFW_PRESS);
          break;
      }
      break;
    }
  }
}

void mousebindingsPoll(void* window, int key, int action, int mods) {
  int k, c;
  for (int i = 0; i < K_MOUSE_BINDS; i++) {
    k = Mousebinds[i].key;
    c = Mousebinds[i].callback;

    if (key == k) {
      switch (c) {
        case NO_CALLBACK:
          Mousebinds[i].pressed =
              (action == GLFW_PRESS || action == GLFW_REPEAT);
          break;
        case CALLBACK_TOGGLE:
          Mousebinds[i].pressed = (action == GLFW_PRESS);
          break;
      }
      break;
    }
  }
}

// Attach keybindings to window.
void keybindingsInit(void* window) {
  glfwSetKeyCallback(window, (GLFWkeyfun)keybindingsPoll);
  glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)(mousebindingsPoll));
}

/*
 * ===========================
 * @WINDOWING, GLFW, GLAD INIT
 * ===========================
 */

typedef struct {
  GLFWwindow* window;
  GLFWmonitor* monitor;
  int resy, resx;
  int fullscreen;
  int vsync;
} Window;

Window WINDOW;

#define WINDOW_INIT \
  {.window = NULL,  \
   .monitor = NULL, \
   .resy = 0,       \
   .resx = 0,       \
   .fullscreen = 1, \
   .vsync = 1}

#define WIN_FALLBACK_X 1920
#define WIN_FALLBACK_Y 1080

#define GL_V_MAJOR 3
#define GL_V_MINOR 3

void pCamOnResolutionChange(int width, int height);

static GLFWwindow* windowCreateFallback() {
  return glfwCreateWindow(WIN_FALLBACK_X, WIN_FALLBACK_Y, TITLE, NULL, NULL);
}

void windowOnSizeChange(void* window, int x, int y) {
  glViewport(0, 0, x, y);
  pCamOnResolutionChange(x, y);
}

// set up opengl and glad contexts and launch a default fullscreen window,
// intended to only be run during the lifetime of the application.
Result windowInit() {
  if (!glfwInit()) {
    log_warn("Failed to initialize GLFW!");
    return Err;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_V_MAJOR);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_V_MINOR);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  const GLFWvidmode* mode;

  WINDOW.monitor = glfwGetPrimaryMonitor();

  if (!WINDOW.monitor || (mode = glfwGetVideoMode(WINDOW.monitor)) == NULL) {
    log_warn(
        "Unable to get video mode information. Setting to fallback resolution "
        "of %d x %d...",
        WIN_FALLBACK_X, WIN_FALLBACK_Y);
    WINDOW.window = windowCreateFallback();
  } else {
    /* WINDOW.window = glfwCreateWindow(mode->width, mode->height, TITLE, */
    /*                                  WINDOW.monitor, NULL); */
    WINDOW.window = windowCreateFallback();
  }

  if (!WINDOW.window) {
    log_error("Unable to create GLFW window!");
    return Err;
  }

  glfwMakeContextCurrent(WINDOW.window);
  glfwGetFramebufferSize(WINDOW.window, &WINDOW.resx, &WINDOW.resy);
  glfwSetFramebufferSizeCallback(WINDOW.window,
                                 (GLFWframebuffersizefun)windowOnSizeChange);

  glfwSetInputMode(WINDOW.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (WINDOW.resx == 0) {
    log_error("failed to query screen coordinates");
    return Err;
  }

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    log_error("Failed to initialize GLAD!");
    return Err;
  }

  keybindingsInit(WINDOW.window);
  mouseInit(WINDOW.window);

  glfwSwapInterval(1);
  glViewport(0, 0, WINDOW.resx, WINDOW.resy);

  return Ok;
}

// Prepare for rendering
void windowNewFrame() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Get inputs from player
void windowPoll() { glfwPollEvents(); }

// End rendering
void windowEndFrame() { glfwSwapBuffers(WINDOW.window); }

// Check if window should close
int windowShouldClose() { return glfwWindowShouldClose(WINDOW.window); }

// Kill anything needed to kill the window
void windowTerminate() { glfwTerminate(); }

/*
 * =======
 * @CAMERA
 * =======
 */

enum CameraMode {
  CAM_TOPDOWN,
  CAM_FPS,
};

typedef struct PerspectiveCamera {
  float pitch, yaw;
  bool firstInt;
  double lastx, lasty;
  vec3 pos, direction, up, front, right;
  mat4 proj, view, ortho;
  float fov;
  float sensitivity;
  int mode;
} PerspectiveCamera;

static PerspectiveCamera pCam;
static float last_pitch;
static vec3 last_pos;

#define pCAM_INIT_POS {0, 4, 20}

void pCamToggleMode(int width, int height) {
  if (pCam.mode == CAM_FPS) {
    pCam.mode = CAM_TOPDOWN;

    last_pitch = pCam.pitch;
    pCam.pitch = -89.0f;

    glm_vec3_copy(pCam.pos, last_pos);
    pCam.pos[1] = 10;
  } else {
    pCam.mode = CAM_FPS;
    glm_vec3_copy(last_pos, pCam.pos);
    pCam.pitch = last_pitch;
  };
}

void pCamUpdateView(int width, int height) {
  pCam.front[0] = cos(glm_rad(pCam.yaw)) * cos(glm_rad(pCam.pitch));
  pCam.front[1] = sin(glm_rad(pCam.pitch));
  pCam.front[2] = sin(glm_rad(pCam.yaw)) * cos(glm_rad(pCam.pitch));
  glm_normalize(pCam.front);

  glm_cross(pCam.front, (vec3){0, 1, 0}, pCam.right);
  glm_normalize(pCam.right);

  glm_cross(pCam.right, pCam.front, pCam.up);
  glm_normalize(pCam.up);

  glm_vec3_add(pCam.pos, pCam.front, pCam.direction);
  glm_lookat(pCam.pos, pCam.direction, pCam.up, pCam.view);
}

void pCamUpdateProj(int width, int height) {
  glm_perspective(glm_rad(pCam.fov), (float)width / height, 0.1, 1000,
                  pCam.proj);
}

void pCamUpdateOrtho(int width, int height) {
  glm_ortho(0, width, 0, height, 0.0, 100, pCam.ortho);
}

void pCamOnFovChange(float fov) {
  pCam.firstInt = true;
  pCam.fov = fov;
  pCamUpdateProj(WINDOW.resx, WINDOW.resy);
  pCamUpdateView(WINDOW.resx, WINDOW.resy);
}

void pCamOnResolutionChange(int width, int height) {
  pCam.firstInt = true;
  pCamUpdateProj(width, height);
  pCamUpdateView(width, height);
  pCamUpdateOrtho(width, height);
}

void pCamInit(int width, int height) {
  pCam = (PerspectiveCamera){0};
  pCam.yaw = -45.0f;
  pCam.pitch = 0.0f;
  pCam.lastx = width / 2.0f;
  pCam.lasty = height / 2.0f;
  pCam.sensitivity = 0.1;
  pCam.fov = 45;
  pCam.firstInt = true;
  pCam.mode = CAM_FPS;
  pCamUpdateProj(width, height);
  pCamUpdateView(width, height);
}

void pCamPan(double xpos, double ypos) {
  if (pCam.firstInt) {
    pCam.lastx = xpos;
    pCam.lasty = ypos;
    pCam.firstInt = false;
  };

  double xoffset = (xpos - pCam.lastx) * pCam.sensitivity;
  double yoffset = (pCam.lasty - ypos) * pCam.sensitivity;

  pCam.lastx = xpos;
  pCam.lasty = ypos;

  pCam.yaw += xoffset;
  pCam.pitch += yoffset;

  pCam.pitch = pCam.pitch > 89.0f ? 89.0f : pCam.pitch;
  pCam.pitch = pCam.pitch < -89.0f ? -89.0f : pCam.pitch;

  pCamUpdateView(WINDOW.resx, WINDOW.resy);
}

/*
 * ========
 * @PHYSICS
 * ========
 *
 */

#define NANOS_PER_SECOND 1000000000

typedef struct {
  double time, last_time, second_frames, fps, last_second;
  double delta;
  struct timespec s;
} Timer;

static Timer TIMER = {0};

uint64_t timeGetNanoseconds() {
  clock_gettime(CLOCK_REALTIME, &TIMER.s);
  return TIMER.s.tv_nsec + (NANOS_PER_SECOND * TIMER.s.tv_sec);
}

void timeInit() {
  uint64_t t = timeGetNanoseconds();
  TIMER.time = t;
  TIMER.last_time = t;
  TIMER.delta = 0;
  TIMER.second_frames = 0;
  TIMER.fps = 0;
  TIMER.last_second = 0;
}

void timeUpdate() {
  TIMER.second_frames++;
  TIMER.time = timeGetNanoseconds();
  TIMER.delta = (TIMER.time - TIMER.last_time) / 1e9f;
  TIMER.last_time = TIMER.time;

  if (TIMER.time - TIMER.last_second >= NANOS_PER_SECOND) {
    TIMER.fps = TIMER.second_frames;
    TIMER.second_frames = 0;
    TIMER.last_second = TIMER.time;
    log_debug("FPS: %f | DELTA: %f", TIMER.fps, TIMER.delta);
  }
}

#define FRICTION_COEFFICIENT 0.85

void attemptMove(vec3 movement, Body* self, Body* colliders, int n_colliders) {
  glm_vec3_scale(movement, FRICTION_COEFFICIENT, movement);

  Body other = {0};
  for (int i = 0; i < n_colliders; i++) {
    glm_vec3_copy(colliders[i].pos, other.pos);
    glm_vec3_add(self->halfsize, colliders[i].halfsize, other.halfsize);

    Hit hit = aabbIntersectRay(self->pos, movement, &other);

    if (hit.is_hit) {
      log_debug("COLLISION!!!!!!!!");
    }
  }

  glm_vec3_add(self->pos, movement, self->pos);
}

/*
 * =========
 * @PICKING
 * =========
 *
 */

void getNormalizedDeviceCoordinates(int resX, int resY, float mouseX,
                                    float mouseY, vec2 dest) {
  float newx = (2.0f * mouseX) / (float)resX - 1.0f;
  float newy = 1.0f - 2.0f * (mouseY / (float)resY);
  dest[0] = newx;
  dest[1] = newy;
};

void clipCoordsToEyeSpace(vec4 clip_coords, mat4 projection, vec4 dest) {
  mat4 inv;
  glm_mat4_inv(projection, inv);

  vec4 eyecoords;
  glm_mat4_mulv(inv, clip_coords, eyecoords);
  dest[0] = eyecoords[0];
  dest[1] = eyecoords[1];
  dest[2] = -1.0f;
  dest[3] = 0.0f;

  // vec_print("Eye coordinates", dest);
}

/// Reverse the eye coordinates to world space
void eyeCoordsToWorldSpace(vec4 eye_coords, mat4 view, vec3 posdest) {
  mat4 inv;
  glm_mat4_inv(view, inv);

  vec4 world_coords;
  glm_mat4_mulv(inv, eye_coords, world_coords);

  glm_vec3_copy(world_coords, posdest);

  // vec_print("World coords", dest);
}

void calculateRayDirection(int width, int height, float x, float y,
                           mat4 projection, mat4 view, vec3 posdest) {
  // normalized device coordinates
  vec2 normalized;
  getNormalizedDeviceCoordinates(width, height, x, y, normalized);

  // clip space
  vec4 clip_space = {normalized[0], normalized[1], -1.0f, 1.0f};

  // eye coordinates
  vec4 eye_coords;
  clipCoordsToEyeSpace(clip_space, projection, eye_coords);

  // world coordinates: A direction into world space.
  eyeCoordsToWorldSpace(eye_coords, view, posdest);
}

#define GET_MOUSE_WORLD_POS(dest)                                         \
  calculateRayDirection(WINDOW.resx, WINDOW.resy, MOUSE.xpos, MOUSE.ypos, \
                        pCam.proj, pCam.view, dest)

/*
 * =======
 * @PLAYER
 * =======
 */

static Body playerBody = {.scale = {1, 1, 1},
                          .halfsize = {1},
                          .pos = {0, 4, 20},
                          .rot = {0, 0, 0},
                          .is_dynamic = true,
                          .is_grounded = false};

/* void playerUpdate(Body* colliders, int n_colliders) { */
void playerUpdate(Body* player_body) {
  vec3 movement = {0, 0, 0};
  float move_speed = 6;

  if (KPRESSED(K_MOVE_FORW)) {
    glm_vec3_add(movement, (vec3){pCam.front[0], 0, pCam.front[2]}, movement);
  }
  if (KPRESSED(K_MOVE_LEFT)) {
    glm_vec3_sub(movement, (vec3){pCam.right[0], 0, pCam.right[2]}, movement);
  }
  if (KPRESSED(K_MOVE_RIGHT)) {
    glm_vec3_add(movement, (vec3){pCam.right[0], 0, pCam.right[2]}, movement);
  }
  if (KPRESSED(K_MOVE_BACK)) {
    glm_vec3_sub(movement, (vec3){pCam.front[0], 0, pCam.front[2]}, movement);
  }

  glm_normalize(movement);
  glm_vec3_scale(movement, move_speed, movement);
  /* glm_vec3_sub(movement, (vec3){0, 9.8, 0}, movement); */
  glm_vec3_copy(movement, player_body->velocity);

  /* attemptMove(movement, &playerBody, colliders, n_colliders); */
  /* glm_vec3_copy(playerBody.pos, pCam.pos); */

  if (KPRESSED(K_EDIT)) {
    pCamToggleMode(WINDOW.resx, WINDOW.resy);
    KRELEASE(K_EDIT);
  }

  if (MPRESSED(K_MOUSE_LEFT)) {
    printf("Pressed left mouse");
    vec3 dest;
    GET_MOUSE_WORLD_POS(dest);
    glm_vec3_print(dest, stderr);
    MRELEASE(K_MOUSE_LEFT);
  }
}

/*
 * =====
 * @TEXT
 * =====
 */
typedef struct Character {
  int textureid;
  vec2 size;
  vec2 bearing;
  unsigned int advance;
} Character;

static unsigned int fontTextureArray;
static Character charMap[128];

#define CHAR_RENDER_BATCH_SIZE 64

static mat4 charMats[CHAR_RENDER_BATCH_SIZE] = {{{1}}};
static int charsToRender[CHAR_RENDER_BATCH_SIZE] = {0};

// clang-format off
float TEXT_VERTICES[] = {
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
};
// clang-format on

Result textInit() {
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    log_error("Failed to initialize freetype2!");
    return Err;
  }

  FT_Face face;
  if (FT_New_Face(ft, "fonts/roboto.ttf", 0, &face)) {
    log_error("Failed to initialize font!");
    return Err;
  }

  FT_Set_Pixel_Sizes(face, 256, 256);
  GL glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  GL glGenTextures(1, &fontTextureArray);
  GL glBindTexture(GL_TEXTURE_2D_ARRAY, fontTextureArray);
  GL glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, 256, 256, 128, 0, GL_RED,
                  GL_UNSIGNED_BYTE, 0);

  GL glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  GL glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  GL glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GL glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  for (unsigned int c = 0; c < 128; c++) {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      log_error("Failed to load char %d", c);
      return Err;
    }

    if (face->glyph->bitmap.buffer) {
      GL glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (int)c,
                         face->glyph->bitmap.width, face->glyph->bitmap.rows, 1,
                         GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
    }

    Character ch = {
        .advance = face->glyph->advance.x,
        .textureid = (int)c,
        .size = {face->glyph->bitmap.width, face->glyph->bitmap.rows},
        .bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top}};

    charMap[c] = ch;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  return Ok;
}

RenderInfo renderTextInit() {
  RenderInfo ri;
  unsigned int vbo;

  ri.shader = shaderFromFileVF("shaders/text.vs", "shaders/text.fs");

  GL glGenVertexArrays(1, &ri.vao);
  GL glGenBuffers(1, &vbo);

  GL glBindVertexArray(ri.vao);
  GL glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GL glBufferData(GL_ARRAY_BUFFER, sizeof(TEXT_VERTICES), TEXT_VERTICES,
                  GL_STATIC_DRAW);

  GL glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  GL glEnableVertexAttribArray(0);

  GL glActiveTexture(GL_TEXTURE0);
  GL glBindVertexArray(0);
  GL glBindBuffer(GL_ARRAY_BUFFER, 0);

  return ri;
}

void _renderText(int length, unsigned int shader) {
  if (length) {
    GL glUniformMatrix4fv(glGetUniformLocation(shader, "transforms"), length,
                          GL_FALSE, &charMats[0][0][0]);

    GL glUniform1iv(glGetUniformLocation(shader, "letterMap"), length,
                    &charsToRender[0]);
    GL glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, length);
  }
}

void renderText(RenderInfo ri, const char* text, float x, float y, float scale,
                vec3 color, int fontpx) {
  scale = scale * fontpx / 256.0f;
  float copyX = x;

  GL glUseProgram(ri.shader);
  shaderSetVec3(ri.shader, "textColor", color);
  shaderSetMat4(ri.shader, "projection", pCam.ortho);
  GL glActiveTexture(GL_TEXTURE0);
  GL glBindTexture(GL_TEXTURE_2D_ARRAY, fontTextureArray);
  GL glBindVertexArray(ri.vao);

  int workingIndex = 0;
  int n = strlen(text);
  Character cur;
  for (int i = 0; i < n; i++) {
    cur = charMap[(int)text[i]];

    if (text[i] == '\n') {
      y -= cur.size[1] * 1.3 * scale;
      x = copyX;
      continue;
    }
    if (text[i] == ' ') {
      x += (cur.advance >> 6) * scale;
      continue;
    }

    float xpos = x + cur.bearing[0] * scale;
    float ypos = y - (256 - cur.bearing[1]) * scale;

    mat4* curMat = &charMats[workingIndex];
    glm_mat4_identity(*curMat);
    glm_translate(curMat[0], (vec3){xpos, ypos, 0});
    glm_scale(*curMat, (vec3){256 * scale, 256 * scale, 1.0f});

    charsToRender[workingIndex] = cur.textureid;

    x += (cur.advance >> 6) * scale;
    workingIndex++;

    if (workingIndex >= CHAR_RENDER_BATCH_SIZE) {
      _renderText(workingIndex, ri.shader);
      workingIndex = 0;
    }
  }
  _renderText(workingIndex, ri.shader);
}

/*
 * ==============
 * @EDITOR SCREEN
 * ==============
 *
 * I want to display a grid of squares representing discrete world coordinates
 * around the view of the player's camera.
 *
 */

#define GRID_MARGIN 10

/* void renderEditGrid(SquareThing* s, RenderInfo ri, vec3 pos, int step_size,
 */
/*                     RenderMatrices rm) { */
/*   Body b = {.rot = {90, 0, 0}, .height = 0.1, .width = 0.1, .pos = {0, 0,
 * 0}}; */

/*   int xstart = pos[0] - GRID_MARGIN, xend = pos[0] + GRID_MARGIN; */
/*   int zstart = pos[2] - GRID_MARGIN, zend = pos[2] + GRID_MARGIN; */

/*   // TODO: make instanced */
/*   for (int i = xstart; i < xend; i += step_size) { */
/*     for (int j = zstart; j < zend; j += step_size) { */
/*       b.pos[0] = i - 0.5; */
/*       b.pos[2] = j - 0.5; */
/*       renderSquare(s, &b, ri, rm, NULL); */
/*     } */
/*   } */
/* } */

/*
 * =========
 * @RENDERER
 * =========
 *
 */

KHASH_MAP_INIT_INT(ri, RenderInfo);

typedef struct Renderer {
  kh_ri_t* renderinfos;  // map render info to int id
  int curid;             // state used to generate IDs for new things
  bool init;
} Renderer;

static Renderer RENDERER = {.renderinfos = NULL, .curid = -1, .init = false};

void rendererInitialize() {
  if (RENDERER.init) {
    return;
  }

  RENDERER.renderinfos = kh_init_ri();

  RENDERER.curid = 0;
  RENDERER.init = true;
};

Result rendererAddThing(Thing* t) {
  if (!RENDERER.init) {
    log_error("Renderer not initialized!");
    return Err;
  }

  khiter_t k;
  RenderInfo ri;
  int ret;

  // Create renderinfo for this item if we haven't already.
  if ((k = kh_get_ri(RENDERER.renderinfos, t->type)) !=
      kh_end(RENDERER.renderinfos)) {
    t->render.ri = kh_val(RENDERER.renderinfos, k);
    log_debug("Already initialized render info for object of type %d: ",
              t->type);
    return Ok;
  }

  ri = (t->render.rinit)();
  k = kh_put_ri(RENDERER.renderinfos, t->type, &ret);

  kh_value(RENDERER.renderinfos, k) = ri;
  t->render.ri = ri;
  return Ok;
}

Result rendererRender(kh_thing_t* things) {
  khiter_t k;
  Thing* t;
  RenderInfo ri;

  for (khint_t i = kh_begin(things); i != kh_end(things); ++i) {
    if (!kh_exist(things, i)) continue;

    t = kh_val(things, i);
    if (!t->render.rfunc) continue;

    (t->render.rfunc)(t->self, &t->body, t->render.ri,
                      (RenderMatrices){.proj = &pCam.proj, .view = &pCam.view},
                      NULL);

    // render bounding box as well.
    if ((t->type == THING_TRIANGLE || t->type == THING_CUBE) &&
        (k = kh_get_ri(RENDERER.renderinfos, THING_CUBE)) !=
            kh_end(RENDERER.renderinfos)) {
      ri = kh_val(RENDERER.renderinfos, k);

      renderAABB(&(CubeThing){.color = {1, 1, 1, 0.2}}, &t->body, ri,
                 (RenderMatrices){&pCam.proj, .view = &pCam.view}, NULL);
    }
  }

  return Ok;
}

/*
 * =====
 * @MAIN
 * =====
 */

int main(void) {
  LOGGER.out = stderr;
  /* pCam = (PerspectiveCamera)pCamInit; */

  WINDOW = (Window)WINDOW_INIT;
  if (is_err(windowInit())) {
    return 1;
  }

  pCamInit(WINDOW.resx, WINDOW.resy);

  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  textInit();
  modelLoaderInit();

  TriangleThing t = {.color = {0, 0, 1, 1}};
  /* SquareThing s = {.color = {0, 0, 1, 0.2}}; */
  CubeThing floor = {.color = {0, 0, 1, 0.2}};
  CubeThing cube = {.color = {0, 0, 1, 0.2}};

  Body floorbody = {
      .pos = {0, 0, 0},
      .scale = {100, 1, 100},
      .rot = {0, 0, 0},
      .is_dynamic = false,
      .velocity = {0, 0, 0},
      .is_grounded = true,
  };

  Body tbody = {
      .pos = {0, 1, -5},
      .scale = {2, 2, 1},
      .rot = {0, 0, 0},
      .is_dynamic = false,
      .velocity = {0, 0, 0},
      .is_grounded = true,
  };

  Body tbody_dynamic = tbody;
  tbody_dynamic.is_dynamic = true;
  tbody_dynamic.pos[1] = 100;
  tbody_dynamic.is_grounded = false;

  Model backpack = {0};

  if (is_err(modelLoadFromFile(&backpack, "meshes/backpack/backpack.obj"))) {
    return 1;
  };

  // TODO: abstract thing generation, renderer addition, and thing manager
  // addition

  Thing* triangle = thingLoadFromData(&t, THING_TRIANGLE, &tbody);
  Thing* triangle2 = thingLoadFromData(&t, THING_TRIANGLE, &tbody_dynamic);
  Thing* floorthing = thingLoadFromData(&floor, THING_CUBE, &floorbody);
  Thing* bpmodel = thingLoadFromData(&backpack, THING_BACKPACK, &tbody);
  Thing* cubething = thingLoadFromData(&cube, THING_CUBE, &tbody);
  Thing* playerthing = thingLoadFromData(NULL, THING_PLAYER, &playerBody);

  timeInit();
  rendererInitialize();
  thingsInit();

  rendererAddThing(triangle);
  rendererAddThing(triangle2);
  rendererAddThing(bpmodel);
  rendererAddThing(floorthing);
  rendererAddThing(cubething);

  thingAdd(triangle);
  thingAdd(triangle2);
  thingAdd(floorthing);
  thingAdd(bpmodel);
  /* thingAdd(cubething); */
  thingAdd(playerthing);

  log_debug("======================");
  log_debug("BEGIN MAIN RENDER LOOP");
  log_debug("======================");

  while (!windowShouldClose()) {
    windowNewFrame();
    windowPoll();
    playerUpdate(&playerthing->body);
    physicsUpdate(THINGS.things, TIMER.delta);

    glm_vec3_copy(playerthing->body.pos, pCam.pos);
    pCamPan(MOUSE.xpos, MOUSE.ypos);

    rendererRender(THINGS.things);

    /* renderText(tri, "Hello there", 300.0f, 300.0f, 1.0f, (vec3){0.5, 0.8,
     * 0.2}, */
    /*            50); */

    windowEndFrame();
    timeUpdate();
  }

  windowTerminate();

  return 0;
}
