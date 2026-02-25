#ifndef GAME_LOG
#define GAME_LOG
/*
 * ========
 * @Logging
 * ========
 */

#include <stdio.h>
#include <glad.h>

typedef enum Result {
  Ok,
  Err,
} Result;

#define is_err(e) e == Err

enum logLevel {
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR,
};

typedef struct Logger {
  char tbuf[16];
  FILE* out;
} Logger;

extern Logger LOGGER;

void glog(int level, int line, const char* file, const char* fmt, ...);

#define log_debug(...) glog(LOG_DEBUG, __LINE__, __FILE__, __VA_ARGS__);
#define log_info(...) glog(LOG_INFO, __LINE__, __FILE__, __VA_ARGS__);
#define log_warn(...) glog(LOG_WARN, __LINE__, __FILE__, __VA_ARGS__)
#define log_error(...) glog(LOG_ERROR, __LINE__, __FILE__, __VA_ARGS__);

GLenum glCheckError_(int line, const char* file);

#define checkGlError() glCheckError_(__LINE__ - 1, __FILE__)
#define GL glCheckError_(__LINE__ - 1, __FILE__);
#endif
