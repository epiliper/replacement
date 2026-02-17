#include <math.h>
#include <stdarg.h>

#define STB_IMAGE_IMPLEMENTATION

#include "glad.h"
#include "GLFW/glfw3.h"
#include "cglm/cglm.h"
#include "kvec.h"
#include "thing.h"
#include "utils.h"
#include "log.h"

#include "mesh.h"

#include "ft2build.h"
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

void mousebindingsPoll(void *window, int key, int action, int mods) {
	int k, c;
	for (int i = 0; i < K_MOUSE_BINDS; i++) {
		k = Mousebinds[i].key;
		c = Mousebinds[i].callback;

		if (key == k) {
			switch (c) {
				case NO_CALLBACK:
					Mousebinds[i].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
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

static int window_created = 0;

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

static GLFWwindow* windowCreateFallback() {
  return glfwCreateWindow(WIN_FALLBACK_X, WIN_FALLBACK_Y, TITLE, NULL, NULL);
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
    WINDOW.window = glfwCreateWindow(mode->width, mode->height, TITLE,
                                     WINDOW.monitor, NULL);
  }

  if (!WINDOW.window) {
    log_error("Unable to create GLFW window!");
    return Err;
  }

  glfwMakeContextCurrent(WINDOW.window);
  glfwGetFramebufferSize(WINDOW.window, &WINDOW.resx, &WINDOW.resy);

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
 * ========
 * @Things + Generics
 * ========
 */

// TODO: in here check if renderinfo has already been initialized.
void thingLoadFromData(void* data, int type, Body loc, Thing* dest) {
  Renderable render;

  // SWTICH STATEMENT FOR TYPE FUNCs
  //

  dest->type = type;
  dest->loc = loc;
  dest->render = render;
}

/*
 * =========
 * @RENDERER
 * =========
 */

typedef kvec_t(Thing) ThingArray;

typedef struct {
  ThingArray things;
} Renderer;

/*
 * ===========
 * @PRIMITIVES
 * ===========
 */

// clang-format off
float TRIANGLE_VERTICES[] = {
    -0.5, -0.5, 0, 
    0.5, -0.5, 0, 
    0.0f, 0.5f, 0,
};

float SQUARE_VERTICES[] = {
	-0.5, -0.5, 0,
	0.5, -0.5, 0,
	0.5, 0.5, 0,
	-0.5, 0.5, 0
};

unsigned int SQUARE_INDICES[] = {
	0, 1, 2,
	2, 3, 0
};

// clang-format on

typedef struct TriangleThing {
  vec4 color;
} TriangleThing;

typedef struct SquareThing {
  vec4 color;
} SquareThing;

const char* triangleVert =
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "uniform mat4 proj;\n"
    "uniform mat4 view;\n"
    "uniform mat4 model;\n"
    "void main() {\n"
    "gl_Position = proj * view * model * vec4(pos.xyz, 1.0f);\n"
    "}";

const char* triangleFrag =
    "#version 330 core\n"
    "out vec4 fragColor;\n"
    "uniform vec4 color;\n"
    "void main() {\n"
    "fragColor = color;\n"
    "}";

RenderInfo renderInitTriangle() {
  RenderInfo ret = {0};

  unsigned int vao, vbo;

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE_VERTICES), TRIANGLE_VERTICES,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  ret.vao = vao;
  ret.shader = shaderFromCharVF(triangleVert, triangleFrag);

  checkGlError();
  return ret;
}

RenderInfo renderSquareInit() {
  RenderInfo ri;

  unsigned int vbo, ebo;
  glGenVertexArrays(1, &ri.vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(ri.vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE_VERTICES), SQUARE_VERTICES,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(SQUARE_INDICES), SQUARE_INDICES,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  ri.shader = shaderFromCharVF(triangleVert, triangleFrag);
  checkGlError();

  return ri;
};

void renderTriangle(TriangleThing* self, Body* body, RenderInfo ri,
                    RenderMatrices rm, RenderMods* mods) {
  GL glBindVertexArray(ri.vao);
  GL glUseProgram(ri.shader);
  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, body->pos);

  glm_rotate(model, glm_rad(body->rot[0]), (vec3){1, 0, 0});
  glm_rotate(model, glm_rad(body->rot[1]), (vec3){0, 1, 0});

  glm_scale(model, (vec3){body->width, body->height, 1});

  shaderSetMat4(ri.shader, "proj", *rm.proj);
  shaderSetMat4(ri.shader, "view", *rm.view);
  shaderSetMat4(ri.shader, "model", model);

  shaderSetVec4(ri.shader, "color", self->color);

  GL glDrawArrays(GL_TRIANGLES, 0, 3);
}

void renderSquare(SquareThing* self, Body* body, RenderInfo ri,
                  RenderMatrices rm, RenderMods* mods) {
  glUseProgram(ri.shader);
  glBindVertexArray(ri.vao);

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, body->pos);

  glm_rotate(model, glm_rad(body->rot[0]), (vec3){1, 0, 0});
  glm_rotate(model, glm_rad(body->rot[1]), (vec3){0, 1, 0});

  glm_scale(model, (vec3){body->width, body->height, 1});

  shaderSetMat4(ri.shader, "proj", *rm.proj);
  shaderSetMat4(ri.shader, "view", *rm.view);
  shaderSetMat4(ri.shader, "model", model);

  shaderSetVec4(ri.shader, "color", self->color);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  checkGlError();
};

struct CubeThing {
  vec4 color;
};

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

#define pCamInit           \
  {.pitch = 0,             \
   .yaw = 0,               \
   .firstInt = true,       \
   .lastx = 0,             \
   .lasty = 0,             \
   .pos = {0, 1, 3},       \
   .direction = {0, 0, 0}, \
   .up = {0, 0, 0},        \
   .right = {0, 0, 0},     \
   .front = {0, 0, -1},    \
   .proj = {0},            \
   .view = {0},            \
   .ortho = {0},           \
   .fov = 45,              \
   .mode = CAM_FPS,        \
   .sensitivity = 0.1}

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
  pCam.fov = fov;
  pCamUpdateProj(WINDOW.resx, WINDOW.resy);
  pCamUpdateView(WINDOW.resx, WINDOW.resy);
}

void pCamOnResolutionChange(int width, int height) {
  pCamUpdateProj(width, height);
  pCamUpdateView(width, height);
  pCamUpdateOrtho(width, height);
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

enum {
  CMOVE_LEFT,
  CMOVE_RIGHT,
  CMOVE_FORW,
  CMOVE_BACK,
};

void pCamMove(int direction) {
  float velocity = 0.1;
  vec3 movement = {0};
  switch (direction) {
    case CMOVE_FORW:
      glm_vec3_add(movement, (vec3){pCam.front[0], 0.0, pCam.front[2]},
                   movement);
      break;
    case CMOVE_BACK:
      glm_vec3_sub(movement, (vec3){pCam.front[0], 0.0, pCam.front[2]},
                   movement);
      break;
    case CMOVE_RIGHT:
      glm_vec3_add(movement, pCam.right, movement);
      break;
    case CMOVE_LEFT:
      glm_vec3_sub(movement, pCam.right, movement);
      break;
  }

  glm_normalize(movement);
  glm_vec3_scale(movement, velocity, movement);
  glm_vec3_add(pCam.pos, movement, pCam.pos);
}

/*
 * =========
 * @PICKING
 * =========
 *
 */

typedef struct Ray {
	vec3 start;
	vec3 end;
};

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
void eyeCoordsToWorldSpace(vec4 eye_coords, mat4 view,
                           vec3 posdest) {
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

#define GET_MOUSE_WORLD_POS(dest) \
	calculateRayDirection(WINDOW.resx, WINDOW.resy, MOUSE.xpos, MOUSE.ypos, pCam.proj, pCam.view, dest)


/*
 * =======
 * @PLAYER
 * =======
 */
void updatePlayer() {
  if (KPRESSED(K_MOVE_FORW)) {
    pCamMove(CMOVE_FORW);
  }
  if (KPRESSED(K_MOVE_LEFT)) {
    pCamMove(CMOVE_LEFT);
  }
  if (KPRESSED(K_MOVE_RIGHT)) {
    pCamMove(CMOVE_RIGHT);
  }
  if (KPRESSED(K_MOVE_BACK)) {
    pCamMove(CMOVE_BACK);
  }

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

static mat4 charMats[CHAR_RENDER_BATCH_SIZE] = {1};
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
    cur = charMap[text[i]];

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

void renderEditGrid(SquareThing* s, RenderInfo ri, vec3 pos, int step_size,
                    RenderMatrices rm) {
  Body b = {.rot = {90, 0, 0}, .height = 0.1, .width = 0.1, .pos = {0, 0, 0}};

  int xstart = pos[0] - GRID_MARGIN, xend = pos[0] + GRID_MARGIN;
  int zstart = pos[2] - GRID_MARGIN, zend = pos[2] + GRID_MARGIN;

  // TODO: make instanced
  for (int i = xstart; i < xend; i += step_size) {
    for (int j = zstart; j < zend; j += step_size) {
      b.pos[0] = i - 0.5;
      b.pos[2] = j - 0.5;
      renderSquare(s, &b, ri, rm, NULL);
    }
  }
}

static int pickedvert = -1;
static int pickedsector = -1;

struct Vertex {
	float x, y;
};

typedef kvec_t(struct Vertex) VertexVec;

struct Sector {
	VertexVec v;
};


void handleEditorClick() {
	// CHECK INTERSECTION HERE.
}

/*
 * =====
 * @MAIN
 * =====
 */

int main(void) {
  LOGGER.out = stderr;
  pCam = (PerspectiveCamera)pCamInit;

  WINDOW = (Window)WINDOW_INIT;
  if (is_err(windowInit())) {
    return 1;
  }

  pCamOnResolutionChange(WINDOW.resx, WINDOW.resy);

  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  textInit();
  modelLoaderInit();
  RenderInfo ri = renderInitTriangle();
  RenderInfo tri = renderTextInit();
  RenderInfo sri = renderSquareInit();

  TriangleThing t = {.color = {0, 0, 1, 1}};
  SquareThing s = {.color = {0, 0, 1, 0.2}};
  Body tbody = {.pos = {0, 0, -5}, .height = 2, .width = 2, .rot = {0, 0, 0}};
  Body sbody = {.pos = {0, -1, -5}, .height = 2, .width = 2, .rot = {90, 0, 0}};

  unsigned int modelShader = shaderFromCharVF(modelVert, modelFrag);
  Model backpack = {0};

  if (is_err(modelLoadFromFile(&backpack, "meshes/backpack/backpack.obj"))) {
    return 1;
  };

  while (!windowShouldClose()) {
    windowNewFrame();
    windowPoll();
    updatePlayer();

    pCamPan(MOUSE.xpos, MOUSE.ypos);

    modelRender(&backpack, &tbody,
                (RenderInfo){.vao = 0, .shader = modelShader},
                (RenderMatrices){&pCam.proj, &pCam.view}, NULL);

    renderEditGrid(&s, sri, pCam.pos, 1,
                   (RenderMatrices){.proj = &pCam.proj, .view = &pCam.view});

    /* renderTriangle(&t, &tbody, ri, */
    /*                (RenderMatrices){.view = &pCam.view, .proj =
     * &pCam.proj},
     */
    /*                NULL); */

    /* renderSquare(&s, &sbody, sri, */
    /*              (RenderMatrices){.view = &pCam.view, .proj = &pCam.proj},
     */
    /*              NULL); */

    /* renderText(tri, "Hello there", 300.0f, 300.0f, 1.0f, (vec3){0.5, 0.8,
     * 0.2}, */
    /*            50); */

    windowEndFrame();
  }

  windowTerminate();

  return 0;
}
