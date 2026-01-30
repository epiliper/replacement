#include "includes/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include "cglm/cglm.h"
#include <time.h>
#include <stdarg.h>
#include "kvec.h"

#define TITLE "replacement"

/*
 * ========
 * @Logging
 * ========
 */

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

void __log(int level, int line, const char *file, const char *fmt, ...) {
	time_t t = time(NULL);
	tbuf[strftime(tbuf, sizeof(tbuf), "%T", localtime(&t))] = '\0';
	fprintf(LOGGER.out, "%s \x1b[1;%sm%s\x1b[22;39m\t%s:%d: ", tbuf, logColors[level], logStrs[level], file, line);

	va_list args;
	va_start(args, fmt);
	vfprintf(LOGGER.out, fmt, args);
	va_end(args);
	fprintf(LOGGER.out, "\n");
};

#define log_debug(...) __log(DEBUG, __LINE__, __FILE__, __VA_ARGS__);
#define log_info(...) __log(INFO, __LINE__, __FILE__, __VA_ARGS__);
#define log_warn(...) __log(WARN, __LINE__, __FILE__, __VA_ARGS__)
#define log_error(...) __log(ERROR, __LINE__, __FILE__, __VA_ARGS__);

/*
 * ===========================
 * @WINDOWING, GLFW, GLAD INIT
 * ===========================
 */

static int window_created = 0;

typedef enum Result {
	Ok,
	Err,
} Result;

#define is_err(e) e == Err

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

	 if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	 {
	 	 log_error("Failed to initialize GLAD!");
		 return Err;
	 }    

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
 * =====
 * @MAIN
 * =====
 */

int main(void) {
	LOGGER.out = stderr;

	Window WINDOW = WINDOW_INIT;
	if (is_err(windowInit())) {
		return 1;
	}

	while (!windowShouldClose()) {
		windowNewFrame();
		windowPoll();
		windowEndFrame();
	}

	windowTerminate();

	return 0;
}

/*
 * ========
 * Renderer
 * ========
 */

typedef struct Renderer {
} Renderer;
