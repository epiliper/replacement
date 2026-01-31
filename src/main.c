#include "glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include "cglm/cglm.h"
#include <time.h>
#include <stdarg.h>
#include "kvec.h"
#include <math.h>

#define TITLE "replacement"

/*
 * ========
 * @Logging
 * ========
 */

typedef enum Result {
	Ok,
	Err,
} Result;

#define is_err(e) e == Err

static char tbuf[16];

enum logLevel {
	DEBUG,
	INFO,
	WARN,
	ERROR,
};

const char* logStrs[] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
};

const char* logColors[] = {
	"37",
	"32",
	"33",
	"31", 
	"31",
};

typedef struct Logger {
	char tbuf[16];
	FILE *out; 
} Logger;

Logger LOGGER;

void glog(int level, int line, const char *file, const char *fmt, ...) {
	time_t t = time(NULL);
	tbuf[strftime(tbuf, sizeof(tbuf), "%T", localtime(&t))] = '\0';
	fprintf(LOGGER.out, "%s \x1b[1;%sm%s\x1b[22;39m\t%s:%d: ", tbuf, logColors[level], logStrs[level], file, line);

	va_list args;
	va_start(args, fmt);
	vfprintf(LOGGER.out, fmt, args);
	va_end(args);
	fprintf(LOGGER.out, "\n");
};

#define log_debug(...) glog(DEBUG, __LINE__, __FILE__, __VA_ARGS__);
#define log_info(...) glog(INFO, __LINE__, __FILE__, __VA_ARGS__);
#define log_warn(...) glog(WARN, __LINE__, __FILE__, __VA_ARGS__)
#define log_error(...) glog(ERROR, __LINE__, __FILE__, __VA_ARGS__);

GLenum glCheckError_(int line, const char *file)
{
    GLenum errorCode;
    const char *header = "GL ERROR:";
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  glog(ERROR, line, file, "%s %s", header, "INVALID_ENUM"); break;
            case GL_INVALID_VALUE:                 glog(ERROR, line, file, "%s %s", header, "INVALID_VALUE"); break;
            case GL_INVALID_OPERATION:             glog(ERROR, line, file, "%s %s", header, "INVALID_OPERATION"); break;
            case GL_OUT_OF_MEMORY:                 glog(ERROR, line, file, "%s %s", header, "OUT_OF_MEMORY"); break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: glog(ERROR, line, file, "%s %s", header, "INVALID_FRAMEBUFFER_OPERATION"); break;
        }
    }
    return errorCode;
}

#define checkGlError() glCheckError_(__LINE__, __FILE__)

/*
 * ============
 * @KEYBINDINGS
 * ============
 *
 * - We need to be able to fetch keybind by action, and action associated with keybind. 
 * - We also need to be able to support keys that just fire as long as they are held, as well as those with specific callbacks.
 * - We need to iterate over both the above groups in one loop.
 */

typedef struct Mouse {
	double xpos, ypos;
} Mouse;

Mouse MOUSE;

void mousePosUpdate(void *window, double xpos, double ypos) {
	/* log_debug("%f %f", MOUSE.xpos, MOUSE.ypos); */
	MOUSE.xpos = xpos;
	MOUSE.ypos = ypos;
}

void mouseInit(void *window) {
	glfwSetCursorPosCallback(window, (GLFWcursorposfun)mousePosUpdate);
}

enum {
	KMOVE_FORW,
	KMOVE_LEFT,
	KMOVE_RIGHT,
	KMOVE_BACK,
	KJUMP,

	KPAUSE,

	K_SHOOT,
	K_AIM,
	K_RELOAD,

	K_BINDS, // number of binds
};

// default keybindings
//

typedef struct {
	int callback;
	int key;
	int pressed;
} Keybinding;

#define KEYBIND(_key, _callback) {.key = _key, .callback = _callback, .pressed = 0}

enum {
	NO_CALLBACK,
	CALLBACK_TOGGLE,
};

Keybinding Keybinds[K_BINDS] = {
KEYBIND(GLFW_KEY_W,								NO_CALLBACK), // forward
KEYBIND(GLFW_KEY_A, 							NO_CALLBACK), // left
KEYBIND(GLFW_KEY_D, 							NO_CALLBACK), // right
KEYBIND(GLFW_KEY_S, 							NO_CALLBACK), // back
KEYBIND(GLFW_KEY_SPACE,						NO_CALLBACK), // jump

KEYBIND(GLFW_KEY_PAUSE,						CALLBACK_TOGGLE), // pause

KEYBIND(GLFW_MOUSE_BUTTON_LEFT,		CALLBACK_TOGGLE), // shoot
KEYBIND(GLFW_MOUSE_BUTTON_RIGHT,	CALLBACK_TOGGLE), // aim
KEYBIND(GLFW_KEY_R,								CALLBACK_TOGGLE), // reload
};

// Check for key presses and respect callbacks.
// TODO: we check every single entry on a keypress. Create a mapping between key and action to save checks.
void keybindingsPoll(void *window, int key, int action, int mods) {
	int k, c;
	for (int i = 0; i < K_BINDS; i++) {
		k = Keybinds[i].key;
		c = Keybinds[i].callback;

		switch (c) {
			case NO_CALLBACK:
				if (key == k) {
					Keybinds[key].pressed = 1;
				} else {
					Keybinds[key].pressed = 0;
				}
				break;

			case CALLBACK_TOGGLE:
				if (key == k && action == GLFW_PRESS) {
					Keybinds[key].pressed = 1;
				} else {
					Keybinds[key].pressed = 0;
				}
				break;
		}
	}
}

// Attach keybindings to window.
void keybindingsInit(void *window) {
	glfwSetKeyCallback(window, (GLFWkeyfun)keybindingsPoll);
	glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)(keybindingsPoll));
}

/*
 * ===========================
 * @WINDOWING, GLFW, GLAD INIT
 * ===========================
 */

static int window_created = 0;

typedef struct {
	GLFWwindow *window;
	GLFWmonitor *monitor;
	int resy, resx;
	int fullscreen;
} Window;

Window WINDOW;

#define WINDOW_INIT {.window = NULL, .monitor = NULL, .resy = 0, .resx = 0, .fullscreen = 1 }

#define WIN_FALLBACK_X 1920
#define WIN_FALLBACK_Y 1080

#define GL_V_MAJOR 3
#define GL_V_MINOR 3

static GLFWwindow *windowCreateFallback() {
	return glfwCreateWindow(WIN_FALLBACK_X, WIN_FALLBACK_Y, TITLE, NULL, NULL);
}


// set up opengl and glad contexts and launch a default fullscreen window, intended to only be run during the lifetime of the application.
Result windowInit() {
 	 if (!glfwInit()) {
 	 	 log_warn("Failed to initialize GLFW!");
 	 	 return Err;
 	 }

 	 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_V_MAJOR);
 	 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_V_MINOR);
	 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 	 const GLFWvidmode *mode;

 	 WINDOW.monitor = glfwGetPrimaryMonitor();

 	 if (!WINDOW.monitor || (mode = glfwGetVideoMode(WINDOW.monitor)) == NULL) {
 	 	 log_warn("Unable to get video mode information. Setting to fallback resolution of %d x %d...", WIN_FALLBACK_X, WIN_FALLBACK_Y);
		 WINDOW.window = windowCreateFallback();
 	 } else {
 	 	 WINDOW.window = glfwCreateWindow(mode->width, mode->height, TITLE, WINDOW.monitor, NULL);
	 }

	 if (!WINDOW.window) {
	 	 log_error("Unable to create GLFW window!");
	 	 return Err;
	 }

	 glfwMakeContextCurrent(WINDOW.window);
	 glfwGetFramebufferSize(WINDOW.window, &WINDOW.resx, &WINDOW.resy);

	 if (WINDOW.resx == 0){
	 	 log_error("failed to query screen coordinates");
	 	 return Err;
	 }

	 if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	 {
	 	 log_error("Failed to initialize GLAD!");
		 return Err;
	 }    

	 keybindingsInit(WINDOW.window);
	 mouseInit(WINDOW.window);

	 glViewport(0, 0, WINDOW.resx, WINDOW.resy);

 	 return Ok;
 }

// Prepare for rendering
void windowNewFrame() {
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

// Get inputs from player
void windowPoll() {
	glfwPollEvents();
}

// End rendering
void windowEndFrame() {
	glfwSwapBuffers(WINDOW.window);
}

// Check if window should close
int windowShouldClose() {
	return glfwWindowShouldClose(WINDOW.window);
}

// Kill anything needed to kill the window
void windowTerminate() {
	glfwTerminate();
}

/*
 * ========
 * @Things + Generics
 * ========
 */

// openGL handles to render an object
typedef struct {
	unsigned int vao, shader;
} RenderInfo;

// matrices necessary for rendering
typedef struct {
	mat4 *proj;
	mat4 *view;
} RenderMatrices;

// thing types
enum {
	THING_TRIANGLE
};


// TODO
typedef struct {
} RenderMods;

// physical information about the object being rendered
typedef struct {
	vec3 pos;
	vec3 rot;
	float height, width;
} Body;

// Function to render a particular thing
typedef void (*RenderFunc)(void *self, Body *body, RenderInfo ri, RenderMatrices rm, RenderMods *mods);

// Function to initialize opengl data for a particular thing
typedef RenderInfo (*RenderInitFunc)();

// Interface for something that is renderable. If we haven't already created vaos, vbos, etc. for the type, then we call the union's init func, otherwise we get its renderinfo.
typedef struct {
	RenderFunc rfunc;
	union {
		RenderInfo ri;
		RenderInitFunc rinit;
	};
} Renderable;

// A thing
typedef struct {
	Body loc;
	int type;
	Renderable render;
	void *self;
	uint16_t id;
} Thing;

// TODO: in here check if renderinfo has already been initialized.
void thingLoadFromData(void *data, int type, Body loc, Thing *dest) {
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

static int success;
static char shaderLog[512];

/* ========
 * @SHADERS
 * =======
 */

unsigned int shaderFromCharVF(const char *vertcode, const char *fragcode) {
  unsigned int frag, vert;

  vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert, 1, &vertcode, NULL);
  glCompileShader(vert);

  glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vert, 512, NULL, shaderLog);
    log_error("Error compiling vertex shader: %s\n", shaderLog);
  }

  frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag, 1, &fragcode, NULL);
  glCompileShader(frag);

  glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(frag, 512, NULL, shaderLog);
    log_error("Error compiling frag shader: %s\n", shaderLog);
  }

  unsigned int shader = glCreateProgram();
  glAttachShader(shader, vert);
  glAttachShader(shader, frag);

  glLinkProgram(shader);

  glDeleteShader(vert);
  glDeleteShader(frag);

  return shader;
}

void shaderSetVec4(unsigned int shader, const char *uni, vec4 dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  glUseProgram(shader);
  glUniform4fv(loc, 1, dat);
}

void shaderSetMat4(unsigned int shader, const char *uni, mat4 dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  if (loc == -1) {
    printf("Failed to find uniform %s\n", uni);
  }
  glUseProgram(shader);
  glUniformMatrix4fv(loc, 1, GL_FALSE, (float *)dat);
}

void shaderSetVec3(unsigned int shader, const char *uni, vec3 dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  if (loc == -1) {
    printf("Failed to find uniform %s\n", uni);
  }
  glUseProgram(shader);
  glUniform3fv(loc, 1, dat);
}

void shaderSetFloat(unsigned int shader, const char *uni, float dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  if (loc == -1) {
    printf("Failed to find uniform %s\n", uni);
  }
  glUseProgram(shader);
  glUniform1f(loc, dat);
}

void shaderSetUnsignedInt(unsigned int shader, const char *uni,
                          unsigned int dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  if (loc == -1) {
    printf("Failed to find uniform %s\n", uni);
  }
  glUseProgram(shader);
  glUniform1ui(loc, dat);
}

/* 
 * ===========
 * @PRIMITIVES
 * ===========
 */

float TRIANGLE_VERTICES[] = {
	-0.5, -0.5, 0,
	0.5, -0.5, 0,
	0.0f, 0.5f, 0,
};

typedef struct TriangleThing {
	vec4 color;
} TriangleThing;

const char *triangleVert = "#version 330 core\n"
"layout (location = 0) in vec3 pos;\n"
"uniform mat4 proj;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"void main() {\n"
"gl_Position = proj * view * model * vec4(pos.xyz, 1.0f);\n"
/* "gl_Position = model * vec4(pos.xyz, 1.0f);\n" */
"}";

const char *triangleFrag = "#version 330 core\n"
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE_VERTICES), TRIANGLE_VERTICES, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	ret.vao = vao;
	ret.shader = shaderFromCharVF(triangleVert, triangleFrag);

	checkGlError();
	return ret;
}

void renderTriangle(TriangleThing *self, Body *body, RenderInfo ri, RenderMatrices rm, RenderMods *mods) {
	glBindVertexArray(ri.vao);
	glUseProgram(ri.shader);

	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, body->pos);
	glm_scale(model, (vec3){body->width, body->height, 1});

	shaderSetMat4(ri.shader, "proj", *rm.proj);
	shaderSetMat4(ri.shader, "view", *rm.view);
	shaderSetMat4(ri.shader, "model", model);

	shaderSetVec4(ri.shader, "color", self->color);

	glDrawArrays(GL_TRIANGLES, 0, 3);
	checkGlError();
}

struct CubeThing {
	vec4 color;
};

/*
 * =======
 * @CAMERA
 * =======
 */

typedef struct PerspectiveCamera {
	float pitch, yaw;
	bool firstInt;
	double lastx, lasty;
	vec3 pos, direction, up, front, right;
	mat4 proj, view;
	float fov;
	float sensitivity;
} PerspectiveCamera;

static PerspectiveCamera pCam;

#define pCamInit {\
	.pitch = 0, .yaw = 0, \
	.firstInt = true, \
	.lastx = 0, .lasty = 0, \
	.pos = {0, 1, 3}, \
	.direction = {0, 0, 0}, \
	.up = {0, 0, 0}, \
	.right = {0, 0, 0}, \
	.front = {0, 0, -1}, \
	.proj = {0}, .view = {0}, \
	.fov = 90, \
	.sensitivity = 0.1 \
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
	glm_perspective(glm_rad(pCam.fov), (float)width / height, 0.1, 1000, pCam.proj);
}

void pCamOnFovChange(float fov) {
	pCam.fov = fov;
	pCamUpdateProj(WINDOW.resx, WINDOW.resy);
	pCamUpdateView(WINDOW.resx, WINDOW.resy);
}

void pCamOnResolutionChange(int width, int height) {
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

enum {
	CMOVE_LEFT, 
	CMOVE_RIGHT,
	CMOVE_FORW,
	CMOVE_BACK,
};

void pCamMove(int direction, float amount) {
	float velocity = 0.1;
	vec3 movement = {0};
	switch (direction) {
		case CMOVE_FORW:
			glm_vec3_add(movement, (vec3){pCam.front[0], 0.0,pCam.front[2]}, movement);
			break;
		case CMOVE_BACK:
			glm_vec3_sub(movement, (vec3){pCam.front[0], 0.0,pCam.front[2]}, movement);
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

	RenderInfo ri = renderInitTriangle();
	TriangleThing t = {.color = {0, 0, 1, 1}};
	Body tbody = {.pos = {0, 0, -5}, .height = 2, .width = 2};


	while (!windowShouldClose()) {
		windowNewFrame();
		windowPoll();

		pCamPan(MOUSE.xpos, MOUSE.ypos);

		renderTriangle(&t, &tbody, ri, (RenderMatrices){.view = &pCam.view, .proj = &pCam.proj}, NULL);

		windowEndFrame();
	}

	windowTerminate();

	return 0;
}


