#include "glad.h"
#include "utils.h"
#include "log.h"

#include <string.h>
/*
 * ========
 * @SHADERS
 * =======
 */

static int success;
static char shaderLog[512];

enum {
  LEFT,
  RIGHT,
};

const char* rSplitOnce(const char* input, const char* delim, int side) {
  int delimlen = strlen(delim);
  int found = 0;
  int i = 0;

  int d = delimlen - 1;

  for (i = strlen(input) - 1; i >= 0; i--) {
    if (input[i] == delim[d]) {
      d--;
      if (d < 0) {
        found = 1;
        break;
      }
    } else {
      d = delimlen - 1;
    }
  }

  if (found) {
    if (side == RIGHT) {
      return strdup(&input[i + 1]);
    } else {
      char* ret = malloc(sizeof(char) * (i + 1));
      memcpy(ret, input, sizeof(char) * (i + 1));
      ret[i] = '\0';
      return ret;
    }
  }

  return NULL;
}

unsigned int shaderFromFileVF(const char* vertfile, const char* fragfile) {
  unsigned int frag, vert;
  const char* src_string;

  int sourcelen;
  src_string = readFileToEnd(vertfile, &sourcelen);
  vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert, 1, &src_string, NULL);
  glCompileShader(vert);

  glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vert, 512, NULL, shaderLog);
    log_error("Error compiling vertex shader: %s", shaderLog);
  }
  free((char*)src_string);

  src_string = readFileToEnd(fragfile, &sourcelen);
  frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag, 1, &src_string, NULL);
  glCompileShader(frag);

  glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(frag, 512, NULL, shaderLog);
    log_error("Error compiling fragment shader: %s", shaderLog);
  }

  free((char*)src_string);

  unsigned int shader = glCreateProgram();
  glAttachShader(shader, vert);
  glAttachShader(shader, frag);

  glLinkProgram(shader);

  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader, 512, NULL, shaderLog);
    log_error("SHADER LINKING FAILED:\n%s\n", shaderLog);
  }

  glDeleteShader(vert);
  glDeleteShader(frag);

  return shader;
}

unsigned int shaderFromCharVF(const char* vertcode, const char* fragcode) {
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

  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader, 512, NULL, shaderLog);
    log_error("SHADER LINKING FAILED:\n%s\n", shaderLog);
  }

  glDeleteShader(vert);
  glDeleteShader(frag);

  return shader;
}

void shaderSetVec4(unsigned int shader, const char* uni, vec4 dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  /* glUseProgram(shader); */
  glUniform4fv(loc, 1, dat);
}

void shaderSetMat4(unsigned int shader, const char* uni, mat4 dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  /* if (loc == -1) { */
  /*   printf("Failed to find uniform %s\n", uni); */
  /* } */
  /* glUseProgram(shader); */
  glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)dat);
}

void shaderSetVec3(unsigned int shader, const char* uni, vec3 dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  /* if (loc == -1) { */
  /*   printf("Failed to find uniform %s\n", uni); */
  /* } */
  /* glUseProgram(shader); */
  glUniform3fv(loc, 1, dat);
}

void shaderSetFloat(unsigned int shader, const char* uni, float dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  /* if (loc == -1) { */
  /*   printf("Failed to find uniform %s\n", uni); */
  /* } */
  /* glUseProgram(shader); */
  glUniform1f(loc, dat);
}

void shaderSetUnsignedInt(unsigned int shader, const char* uni,
                          unsigned int dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  /* if (loc == -1) { */
  /*   printf("Failed to find uniform %s\n", uni); */
  /* } */
  /* glUseProgram(shader); */
  glUniform1ui(loc, dat);
}

void shaderSetInt(unsigned int shader, const char* uni, int dat) {
  GLint loc = glGetUniformLocation(shader, uni);
  /* if (loc == -1) { */
  /*   printf("Failed to find uniform %s\n", uni); */
  /* } */
  /* glUseProgram(shader); */
  glUniform1i(loc, dat);
}

/*
 * =====
 * @FILE
 * =====
 */

const char* readFileToEnd(const char* path, int* n) {
  FILE* f = fopen(path, "rb");  // Use "rb" for binary mode
  if (!f) {
    log_error("Failed to open: %s\n", path);
    *n = 0;
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  *n = ftell(f);
  rewind(f);

  char* ret = malloc(*n + 1);
  if (!ret) {
    fclose(f);
    return NULL;
  }

  size_t read = fread(ret, 1, *n, f);
  if (read != *n) {
    log_error("Partial read of %s: got %zu, expected %d\n", path, read, *n);
  }

  ret[*n] = '\0';
  fclose(f);

  return ret;
}
