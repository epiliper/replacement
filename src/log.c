#include "log.h"
#include <time.h>
#include <stdarg.h>
#include "glad.h"

Logger LOGGER;

const char* logStrs[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
};

const char* logColors[] = {
    "37", "32", "33", "31", "31",
};

void glog(int level, int line, const char* file, const char* fmt, ...) {
  time_t t = time(NULL);
  LOGGER.tbuf[strftime(LOGGER.tbuf, sizeof(LOGGER.tbuf), "%T", localtime(&t))] =
      '\0';
  fprintf(LOGGER.out, "%s \x1b[1;%sm%s\x1b[22;39m\t%s:%d: ", LOGGER.tbuf,
          logColors[level], logStrs[level], file, line);

  va_list args;
  va_start(args, fmt);
  vfprintf(LOGGER.out, fmt, args);
  va_end(args);
  fprintf(LOGGER.out, "\n");
};

GLenum glCheckError_(int line, const char* file) {
  GLenum errorCode;
  const char* header = "GL ERROR:";
  while ((errorCode = glGetError()) != GL_NO_ERROR) {
    switch (errorCode) {
      case GL_INVALID_ENUM:
        glog(LOG_ERROR, line, file, "%s %s", header, "INVALID_ENUM");
        break;
      case GL_INVALID_VALUE:
        glog(LOG_ERROR, line, file, "%s %s", header, "INVALID_VALUE");
        break;
      case GL_INVALID_OPERATION:
        glog(LOG_ERROR, line, file, "%s %s", header, "INVALID_OPERATION");
        break;
      case GL_OUT_OF_MEMORY:
        glog(LOG_ERROR, line, file, "%s %s", header, "OUT_OF_MEMORY");
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        glog(LOG_ERROR, line, file, "%s %s", header,
             "INVALID_FRAMEBUFFER_OPERATION");
        break;
    }
  }
  return errorCode;
}
