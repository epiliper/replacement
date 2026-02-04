#ifndef GMESH
#define GMESH

#include "cglm/cglm.h"
#include "kvec.h"
#include "thing.h"
#include "log.h"

#include "stbi_image.h"

typedef struct MeshVertex {
  vec3 pos;
  vec3 normals;
  vec2 texcoords;
  vec3 tangent;
  vec3 bitangent;
} MeshVertex;

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

typedef kvec_t(Mesh) MeshVec;

typedef struct Model {
  MeshVec meshes;
  const char *directory;
} Model;

extern const char* modelVert;
extern const char* modelFrag;

void modelLoaderInit();
void modelDraw(Model *m);
Result modelLoadFromFile(Model *model, char *path);

#endif
