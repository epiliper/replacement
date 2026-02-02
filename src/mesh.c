#include "cglm/cglm.h"
#include "glad.h"
#include "kvec.h"
#include "thing.h"
#include "stdio.h"

#include <assimp/scene.h>
#include <stddef.h>

typedef struct MeshVertex {
  vec3 pos;
  vec3 normals;
  vec2 texcoords;
} MeshVertex;

enum TEXTURE_TYPE {
  T_SPECULAR,
  T_DIFFUSE,
  T_OTHER,
  T_TYPES,
};

const char* textureNames[T_TYPES] = {
    "texture_specular",
    "texture_diffuse",
    "texture_other",
};

typedef struct MeshTexture {
  unsigned int id;
  int type;
} MeshTexture;

typedef kvec_t(MeshVertex) mVertVec;
typedef kvec_t(MeshTexture) mTexVec;
typedef kvec_t(unsigned int) mIndVec;

typedef struct Mesh {
  mVertVec vertices;
  mTexVec textures;
  mIndVec indices;
  RenderInfo ri;
} Mesh;

void meshSetup(Mesh* dest) {
  unsigned int vbo, ebo;
  glGenVertexArrays(1, &dest->ri.vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(dest->ri.vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, dest->vertices.n * sizeof(MeshVertex),
               dest->vertices.a, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, dest->vertices.n * sizeof(unsigned int),
               dest->vertices.a, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (void*)offsetof(MeshVertex, normals));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (void*)offsetof(MeshVertex, texcoords));
}

void meshDraw(Mesh* m) {
  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  char uniform[256];

  for (unsigned int i = 0; i < m->textures.n; i++) {
    glActiveTexture(GL_TEXTURE0 + i);

    switch (m->textures.a[i].type) {
      case (T_DIFFUSE):
        snprintf(uniform, 256, "material.%s%d", textureNames[T_DIFFUSE],
                 diffuseNr);
        diffuseNr++;
        break;
      case (T_SPECULAR):
        snprintf(uniform, 256, "material.%s%d", textureNames[T_SPECULAR],
                 specularNr);
        specularNr++;
        break;
      case (T_OTHER):
        break;
    }
  }
}
