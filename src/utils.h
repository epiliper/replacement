#ifndef GAME_UTILS
#define GAME_UTILS
#include "cglm/cglm.h"

/*
 * ========
 * @SHADER
 * =======
 */

unsigned int shaderFromFileVF(const char* vertfile, const char* fragfile);

unsigned int shaderFromCharVF(const char* vertcode, const char* fragcode);

void shaderSetVec4(unsigned int shader, const char* uni, vec4 dat);
void shaderSetMat4(unsigned int shader, const char* uni, mat4 dat);
void shaderSetVec3(unsigned int shader, const char* uni, vec3 dat);
void shaderSetFloat(unsigned int shader, const char* uni, float dat);
void shaderSetUnsignedInt(unsigned int shader, const char* uni,
                          unsigned int dat);

/*
 * =====
 * @FILE
 * =====
 *
 */

const char* readFileToEnd(const char* path, int* n);
#endif
